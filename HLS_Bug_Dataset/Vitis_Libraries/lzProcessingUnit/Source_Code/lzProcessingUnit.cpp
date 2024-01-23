void lzProcessingUnit(hls::stream<ap_uint<17> >& inStream,
                      hls::stream<SIZE_DT>& litLenStream,
                      hls::stream<SIZE_DT>& matchLenStream,
                      hls::stream<ap_uint<16> >& offsetStream,
                      hls::stream<ap_uint<10> >& outStream) {
    ap_uint<17> inValue, nextValue;
    const int c_maxLitLen = 128;
    uint16_t offset = 0;
    uint16_t matchLen = 0;
    uint8_t litLen = 0;
    uint8_t outLitLen = 0;
    ap_uint<10> lit = 0;

    nextValue = inStream.read();
    bool eosFlag = nextValue.range(0, 0);
    bool lastLiteral = false;
    bool isLiteral = true;
lzProcessing:
    for (; eosFlag == false;) {
#pragma HLS PIPELINE II = 1
        inValue = nextValue;
        nextValue = inStream.read();
        eosFlag = nextValue.range(0, 0);

        bool outFlag, outStreamFlag;
        if (inValue.range(16, 9) == 0xFF && isLiteral) {
            outStreamFlag = true;
            outLitLen = litLen + 1;
            if (litLen == c_maxLitLen - 1) {
                outFlag = true;
                matchLen = 0;
                offset = 1; // dummy value
                litLen = 0;
            } else {
                outFlag = false;
                litLen++;
            }
        } else {
            if (isLiteral) {
                matchLen = inValue.range(16, 1);
                isLiteral = false;
                outFlag = false;
                outStreamFlag = false;
            } else {
                offset = inValue.range(16, 1);
                isLiteral = true;
                outFlag = true;
                outLitLen = litLen;
                litLen = 0;
                outStreamFlag = false;
            }
        }
        if (outStreamFlag) {
            lit.range(9, 2) = inValue.range(8, 1);
            if (nextValue.range(16, 9) == 0xFF) {
                lit.range(1, 0) = 0;
            } else {
                lit.range(1, 0) = 1;
            }
            lastLiteral = true;
            outStream << lit;
        } else if (lastLiteral) {
            outStream << 3;
            lastLiteral = false;
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
    }

    // Terminate condition
    outStream << 2;
    offsetStream << 0;
    matchLenStream << 0;
    litLenStream << 0;
}