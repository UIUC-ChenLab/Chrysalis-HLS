void fseStreamStatesLL(uint32_t* litFSETable,
                       uint32_t* oftFSETable,
                       uint32_t* mlnFSETable,
                       ap_uint<IN_BYTES * 8>* bitStream,
                       int byteIndx,
                       uint8_t lastByteValidBits,
                       uint32_t seqCnt,
                       uint8_t* accuracyLog,
                       hls::stream<uint8_t>& litsymbolStream,
                       hls::stream<uint8_t>& mlsymbolStream,
                       hls::stream<uint8_t>& ofsymbolStream,
                       hls::stream<ap_uint<32> >& litbsWordStream,
                       hls::stream<ap_uint<32> >& mlbsWordStream,
                       hls::stream<ap_uint<32> >& ofbsWordStream,
                       hls::stream<ap_uint<5> >& litextraBitStream,
                       hls::stream<ap_uint<5> >& mlextraBitStream) {
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
    uint16_t mskLL = ((1 << accuracyLog[0]) - 1);
    fseStateLL = ((acchbs >> (bitsInAcc - accuracyLog[0])) & mskLL);
    uint16_t mskOF = ((1 << accuracyLog[1]) - 1);
    fseStateOF = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1]))) & mskOF);
    uint16_t mskML = ((1 << accuracyLog[2]) - 1);
    fseStateML = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]))) & mskML);

    bitsInAcc -= (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]);

    enum FSEDState_t { READ_FSE = 0, GET_FSE };
    FSEDState_t smState = READ_FSE;
    uint8_t bitCntLML, bitCntLMO;
    uint32_t bswLL, bswML, bswOF;
    // read data to bitstream if necessary
    if (bitsInAcc < c_streamWidth && byteIndx > -1) {
        auto tmp = bitStream[byteIndx--];
        acchbs = (acchbs << c_streamWidth) + tmp;
        bitsInAcc += c_streamWidth;
    }

decode_sequence_bitStream:
    while (seqCnt) {
#pragma HLS PIPELINE II = 1
        uint8_t processedBits = 0;
        if (smState == READ_FSE) {
            // get state values and stream
            // stream fse metadata to decoding unit
            // get the state codes for offset, match_length and literal_length
            ap_uint<32> ofstateVal = oftFSETable[fseStateOF]; // offset
            bsStateOF.symbol = ofstateVal.range(7, 0);
            bsStateOF.bitCount = ofstateVal.range(15, 8);
            bsStateOF.nextState = ofstateVal.range(31, 16);

            uint8_t ofSymBits = bsStateOF.symbol; // also represents extra bits to be read, max 31-bits
            ofsymbolStream << bsStateOF.symbol;
            bswOF = acchbs >> (bitsInAcc - ofSymBits);

            litbsWordStream << bswLL;
            bitCntLMO = bsStateOF.bitCount;

            ap_uint<32> mlstateVal = mlnFSETable[fseStateML]; // match_length
            bsStateML.symbol = mlstateVal.range(7, 0);
            bsStateML.bitCount = mlstateVal.range(15, 8);
            bsStateML.nextState = mlstateVal.range(31, 16);

            uint8_t mlextraBits = c_extraBitsML[bsStateML.symbol]; // max 16-bits
            bswML = acchbs >> (bitsInAcc - ofSymBits - mlextraBits);
            mlsymbolStream << bsStateML.symbol;
            mlextraBitStream << mlextraBits;

            bitCntLML = bsStateML.bitCount;
            bitCntLMO += bsStateML.bitCount;

            ofbsWordStream << bswOF;

            ap_uint<32> litstateVal = litFSETable[fseStateLL]; // literal_length
            bsStateLL.symbol = litstateVal.range(7, 0);
            bsStateLL.bitCount = litstateVal.range(15, 8);
            bsStateLL.nextState = litstateVal.range(31, 16);

            uint8_t litextraBits = c_extraBitsLL[bsStateLL.symbol]; // max 16-bits
            bswLL = acchbs >> (bitsInAcc - ofSymBits - mlextraBits - litextraBits);
            litsymbolStream << bsStateLL.symbol;
            litextraBitStream << litextraBits;

            bitCntLML += bsStateLL.bitCount;
            bitCntLMO += bsStateLL.bitCount;
            mlbsWordStream << bswML;
            processedBits = ofSymBits + litextraBits + mlextraBits;

            mskLL = (((uint16_t)1 << bsStateLL.bitCount) - 1);
            mskML = (((uint16_t)1 << bsStateML.bitCount) - 1);
            mskOF = (((uint16_t)1 << bsStateOF.bitCount) - 1);

            smState = GET_FSE;
        } else if (smState == GET_FSE) {
            processedBits = bitCntLMO;
            // update state for next sequence
            // read bits to get states for literal_length, match_length, offset
            // accumulator must contain these many bits can be max 26-bits
            fseStateLL = ((acchbs >> (bitsInAcc - bsStateLL.bitCount)) & mskLL);
            fseStateLL += bsStateLL.nextState;

            fseStateML = ((acchbs >> (bitsInAcc - bitCntLML)) & mskML);
            fseStateML += bsStateML.nextState;

            fseStateOF = ((acchbs >> (bitsInAcc - bitCntLMO)) & mskOF);
            fseStateOF += bsStateOF.nextState;

            --seqCnt;
            smState = READ_FSE;
        }
        // read data to bitstream if necessary
        if ((bitsInAcc - processedBits) < c_streamWidth && byteIndx > -1) {
            auto tmp = bitStream[byteIndx--];
            acchbs = (acchbs << c_streamWidth) + tmp;
            bitsInAcc += c_streamWidth - processedBits;
        } else {
            bitsInAcc -= processedBits;
        }
    }
    litbsWordStream << bswLL;
}