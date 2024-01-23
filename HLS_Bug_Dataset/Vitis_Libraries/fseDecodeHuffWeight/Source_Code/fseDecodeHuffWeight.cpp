void fseDecodeHuffWeight(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                         uint32_t remSize,
                         ap_uint<IN_BYTES * 8 * 2>& accHuff,
                         uint8_t& bytesInAcc,
                         uint8_t accuracyLog,
                         uint32_t* fseTable,
                         uint8_t* weights,
                         uint16_t& weightCnt,
                         uint8_t& huffDecoderTableLog) {
    //#pragma HLS INLINE
    const uint8_t c_inputByte = IN_BYTES;
    const uint8_t c_streamWidth = 8 * c_inputByte;
    uint8_t bitsInAcc = bytesInAcc * 8;

    uint8_t bitStream[128];
    int bsByteIndx = remSize - 1;

    // copy data from bitstream to buffer
    uint32_t itrCnt = 1 + ((remSize - 1) / c_inputByte);
    uint32_t k = 0;
    ap_uint<c_streamWidth> input;

fseDecHF_read_input:
    for (uint32_t i = 0; i < itrCnt; ++i) {
        input = inStream.read();
        bitsInAcc += c_streamWidth;
        for (uint8_t j = 0; j < c_inputByte && k < remSize; ++j, ++k) {
#pragma HLS PIPELINE II = 1
            bitStream[k] = input.range(7, 0);
            input >>= 8;
            bitsInAcc -= 8;
        }
    }
    accHuff = input;
    bytesInAcc = bitsInAcc >> 3;

    // decode FSE bitStream using fse table to get huffman weights
    // skip initial 0 bits and single 1 bit
    uint32_t accState;
    uint32_t codeIdx = 0;
    uint8_t bitsToRead = 0;
    bitsInAcc = 0;
    int32_t rembits = remSize * 8;

    uint8_t fseState[2];
    FseBSState bsState[2];
    // find beginning of the stream
    accState = bitStream[bsByteIndx--];
    bitsInAcc = 8;
    for (ap_uint<4> i = 7; i >= 0; --i) {
#pragma HLS UNROLL
        if ((accState & (1 << i)) > 0) {
            --bitsInAcc;
            break;
        }
        --bitsInAcc; // discount higher bits which are zero
    }
    rembits -= (8 - bitsInAcc);

    // Read bits needed for first two states
    bitsToRead = accuracyLog * 2;
    if (bitsToRead > bitsInAcc) {
        uint8_t bytesToRead = 1 + ((bitsToRead - bitsInAcc - 1) / 8);
        for (uint8_t i = 0; i < bytesToRead; ++i) {
            uint8_t tmp = bitStream[bsByteIndx--];
            accState <<= 8;
            accState += tmp;
            bitsInAcc += 8;
        }
    }
    // Read initial state1 and state2
    // get readBits bits from higher position in accuracyLog, mask out higher scrap bits
    // read state 1
    bitsInAcc -= accuracyLog;
    uint16_t msk = ((1 << accuracyLog) - 1);
    fseState[0] = ((accState >> bitsInAcc) & msk);
    // read state 2
    bitsInAcc -= accuracyLog;
    msk = ((1 << accuracyLog) - 1);
    fseState[1] = ((accState >> bitsInAcc) & msk);
    rembits -= (accuracyLog * 2);

    bool stateIdx = 0; // 0 for even, 1 for odd
    bool overflow = false;
    uint32_t totalWeights = 0;
    // get the weight, bitCount and nextState
    uint32_t stateVal = fseTable[fseState[stateIdx]];
    uint8_t cw = (uint8_t)(stateVal & 0xFF);
    weights[codeIdx++] = cw;
    totalWeights += (1 << cw) >> 1;
fse_decode_huff_weights:
    for (; rembits >= 0;) {
#pragma HLS PIPELINE II = 1
        // get other values
        bsState[stateIdx].nextState = (stateVal >> 16) & 0x0000FFFF;
        bsState[stateIdx].bitCount = (stateVal >> 8) & 0x000000FF;
        uint8_t bitsToRead = bsState[stateIdx].bitCount;
        if (bitsToRead > bitsInAcc) {
            uint8_t tmp = 0;
            if (bsByteIndx > -1) {
                // max 1 read is required, since accuracy log <= 6
                tmp = bitStream[bsByteIndx--];
            }
            accState <<= 8;
            accState += tmp;
            bitsInAcc += 8;
        }
        // get next fse state
        bitsInAcc -= bitsToRead;
        uint8_t msk = ((1 << bitsToRead) - 1);
        fseState[stateIdx] = ((accState >> bitsInAcc) & msk);
        fseState[stateIdx] += bsState[stateIdx].nextState;
        rembits -= bitsToRead;

        // switch state flow
        stateIdx = (stateIdx + 1) & 1; // 0 if 1, 1 if 0
        stateVal = fseTable[fseState[stateIdx]];
        cw = (uint8_t)(stateVal & 0xFF);
        weights[codeIdx++] = cw;
        totalWeights += (1 << cw) >> 1;
    }
    huffDecoderTableLog = 1 + (31 - __builtin_clz(totalWeights));
    // add last weight
    uint16_t lw = (1 << huffDecoderTableLog) - totalWeights;
    weights[codeIdx++] = 1 + (31 - __builtin_clz(lw));
    weightCnt = codeIdx;
}