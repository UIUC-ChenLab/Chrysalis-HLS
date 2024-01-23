void lzProcessingUnitLL(hls::stream<ap_uint<16> >& inStream,
                        hls::stream<SIZE_DT>& litLenStream,
                        hls::stream<SIZE_DT>& matchLenStream,
                        hls::stream<ap_uint<16> >& offsetStream,
                        hls::stream<ap_uint<10> >& outStream) {
    ap_uint<16> inValue, nextValue;
    const int c_maxLitLen = 128;
    uint16_t offset = 0;
    uint16_t matchLen = 0;
    uint8_t litLen = 0;
    uint8_t outLitLen = 0;
    ap_uint<10> lit = 0;
    const uint16_t lbase[32] = {0,  3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27,
                                31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};

    const uint16_t dbase[32] = {1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
                                49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
                                2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};
    nextValue = inStream.read();
    bool eosFlag = (nextValue == 0xFFFF) ? true : false;
    bool lastLiteral = false;
    bool isLitLength = true;
    bool isExtra = false;
    bool dummyValue = false;
lzProcessing:
    for (; eosFlag == false;) {
#pragma HLS PIPELINE II = 1
        inValue = nextValue;
        nextValue = inStream.read();
        eosFlag = (nextValue == 0xFFFF);

        bool outFlag, outStreamFlag;
        if ((inValue.range(15, 8) == 0xFE) || (inValue.range(15, 8) == 0xFD)) {
            // ignore invalid byte
            outFlag = false;
            outStreamFlag = false;
        } else if (inValue.range(15, 8) == 0xF0) {
            outStreamFlag = true;
            outLitLen = litLen + 1;
            if (litLen == c_maxLitLen - 1) {
                outFlag = true;
                matchLen = 0;
                offset = 1; // dummy value
                litLen = 0;
            } else {
                outFlag = false;
                matchLen = 0;
                offset = 1; // dummy value
                litLen++;
            }
        } else if (isExtra && isLitLength) { // matchLen Extra
            matchLen += inValue.range(15, 0);
            isExtra = false;
            isLitLength = false;
            outStreamFlag = true;

        } else if (isExtra && !isLitLength) { // offset Extra
            offset += inValue.range(15, 0);
            isExtra = false;
            isLitLength = true;
            outFlag = true;
            outStreamFlag = true;
        } else if (isLitLength) {
            auto val = inValue.range(4, 0);
            matchLen = lbase[val];
            if (val < 9) {
                isExtra = false;
                isLitLength = false;
            } else {
                isExtra = true;
                isLitLength = true;
            }
            outFlag = false;
            outStreamFlag = true;
            dummyValue = true;
        } else {
            auto val = inValue.range(4, 0);
            offset = dbase[val];
            if (val < 4) {
                isExtra = false;
                isLitLength = true;
                outFlag = true;

            } else {
                isExtra = true;
                isLitLength = false;
                outFlag = false;
            }

            outLitLen = litLen;
            litLen = 0;
            outStreamFlag = true;
        }

        if (outStreamFlag) {
            lit.range(9, 2) = inValue.range(7, 0);
            if ((inValue.range(15, 8) == 0xF0)) {
                lit.range(1, 0) = 0;
            } else {
                lit.range(1, 0) = 1;
            }
            outStream << lit;
        }

        if (outFlag) {
            litLenStream << outLitLen;
            offsetStream << offset;
            matchLenStream << matchLen;
        }
    }

    if (litLen) {
        litLenStream << litLen;
        offsetStream << 0;
        matchLenStream << 0;
        outStream << 3;
    }

    // Terminate condition
    outStream << 2;
    offsetStream << 0;
    matchLenStream << 0;
    litLenStream << 0;
}