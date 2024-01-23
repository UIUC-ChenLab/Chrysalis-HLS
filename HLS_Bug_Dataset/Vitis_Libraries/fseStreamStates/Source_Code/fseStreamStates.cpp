void fseStreamStates(uint32_t* litFSETable,
                     uint32_t* oftFSETable,
                     uint32_t* mlnFSETable,
                     ap_uint<IN_BYTES * 8>* bitStream,
                     int byteIndx,
                     uint8_t lastByteValidBits,
                     uint32_t seqCnt,
                     uint8_t* accuracyLog,
                     hls::stream<uint8_t>& symbolStream,
                     hls::stream<ap_uint<32> >& bsWordStream,
                     hls::stream<ap_uint<5> >& extraBitStream) {
    // fetch fse states from fse sequence bitstream and stream out for further processing
    const uint8_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const uint8_t c_bsBytes = c_streamWidth / 8;

    uint16_t fseStateLL, fseStateOF, fseStateML; // literal_length, offset, match_length states
    FseBSState bsStateLL, bsStateOF, bsStateML;  // offset, match_length and literal_length
    ap_uint<c_accRegWidth> acchbs;
    uint8_t bitsInAcc = lastByteValidBits;
    uint8_t bitsToRead = 0;
    uint8_t bytesToRead = 0;

    acchbs.range(c_streamWidth - 1, 0) = bitStream[byteIndx--];
    uint8_t byte_0 = acchbs.range(bitsInAcc - 1, bitsInAcc - 8);
// find valid last bit, bitstream read in reverse order
fsedseq_skip_zero:
    for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
        if ((byte_0 & (1 << i)) > 0) {
            --bitsInAcc;
            break;
        }
        --bitsInAcc; // discount higher bits which are zero
    }

fsedseq_fill_acc:
    while ((bitsInAcc + c_streamWidth < c_accRegWidth) && (byteIndx > -1)) {
#pragma HLS PIPELINE II = 1
        acchbs <<= c_streamWidth;
        acchbs.range(c_streamWidth - 1, 0) = bitStream[byteIndx--];
        bitsInAcc += c_streamWidth;
    }
    // Read literal_length, offset and match_length states from input stream
    // get *accuracyLog bits from higher position in accuracyLog, mask out higher scrap bits
    uint64_t mskLL = ((1 << accuracyLog[0]) - 1);
    fseStateLL = ((acchbs >> (bitsInAcc - accuracyLog[0])) & mskLL);
    uint64_t mskOF = ((1 << accuracyLog[1]) - 1);
    fseStateOF = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1]))) & mskOF);
    uint64_t mskML = ((1 << accuracyLog[2]) - 1);
    fseStateML = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]))) & mskML);

    bitsInAcc -= (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]);

    enum FSEDState_t { LITLEN = 0, MATLEN, OFFSET, NEXTSTATE };
    FSEDState_t smState = OFFSET;
    uint8_t bitCntLML, bitCntLMO;
    uint32_t bswLL, bswML, bswOF;

decode_sequence_bitStream:
    while (seqCnt) {
#pragma HLS PIPELINE II = 1
        // read data to bitstream if necessary
        if (bitsInAcc < c_streamWidth && byteIndx > -1) {
            auto tmp = bitStream[byteIndx--];
            acchbs = (acchbs << c_streamWidth) + tmp;
            bitsInAcc += c_streamWidth;
        }
        uint32_t stateVal;
        uint8_t extraBits;
        // get state values and stream
        // stream fse metadata to decoding unit
        if (smState == LITLEN) {
            bsWordStream << bswOF;

            stateVal = litFSETable[fseStateLL]; // literal_length
            bsStateLL.symbol = stateVal & 0x000000FF;
            bsStateLL.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateLL.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = c_extraBitsLL[bsStateLL.symbol]; // max 16-bits
            bitsInAcc -= extraBits;
            symbolStream << bsStateLL.symbol;
            bswLL = acchbs >> bitsInAcc;
            extraBitStream << extraBits;

            bitCntLML += bsStateLL.bitCount;
            bitCntLMO += bsStateLL.bitCount;
            smState = NEXTSTATE;
        } else if (smState == MATLEN) {
            stateVal = mlnFSETable[fseStateML]; // match_length
            bsStateML.symbol = stateVal & 0x000000FF;
            bsStateML.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateML.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = c_extraBitsML[bsStateML.symbol]; // max 16-bits
            bitsInAcc -= extraBits;
            symbolStream << bsStateML.symbol;
            bswML = acchbs >> bitsInAcc;
            extraBitStream << extraBits;

            bitCntLML = bsStateML.bitCount;
            bitCntLMO += bsStateML.bitCount;
            smState = LITLEN;
        } else if (smState == OFFSET) {
            // get the state codes for offset, match_length and literal_length
            stateVal = oftFSETable[fseStateOF]; // offset
            bsStateOF.symbol = stateVal & 0x000000FF;
            bsStateOF.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateOF.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = bsStateOF.symbol; // also represents extra bits to be read, max 31-bits
            bitsInAcc -= extraBits;
            symbolStream << bsStateOF.symbol;
            bswOF = acchbs >> bitsInAcc;

            bsWordStream << bswLL;
            bitCntLMO = bsStateOF.bitCount;
            smState = MATLEN;
        } else {
            bsWordStream << bswML;
            // update state for next sequence
            // read bits to get states for literal_length, match_length, offset
            // accumulator must contain these many bits can be max 26-bits
            mskLL = (((uint64_t)1 << bsStateLL.bitCount) - 1);
            fseStateLL = ((acchbs >> (bitsInAcc - bsStateLL.bitCount)) & mskLL);
            fseStateLL += bsStateLL.nextState;

            mskML = (((uint64_t)1 << bsStateML.bitCount) - 1);
            fseStateML = ((acchbs >> (bitsInAcc - bitCntLML)) & mskML);
            fseStateML += bsStateML.nextState;

            mskOF = (((uint64_t)1 << bsStateOF.bitCount) - 1);
            fseStateOF = ((acchbs >> (bitsInAcc - bitCntLMO)) & mskOF);
            fseStateOF += bsStateOF.nextState;

            bitsInAcc -= bitCntLMO;

            --seqCnt;
            smState = OFFSET;
        }
    }
    bsWordStream << bswLL;
}