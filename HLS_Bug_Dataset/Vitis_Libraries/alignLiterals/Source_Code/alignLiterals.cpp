void alignLiterals(hls::stream<bool>& litlenValidStream,
                   hls::stream<bool>& newBlockFlagStream,
                   hls::stream<IntVectorStream_dt<PARALLEL_BYTE * 8, 1> >& inLitStream,
                   hls::stream<ap_uint<LMO_WIDTH> >& inLitLenStream,
                   hls::stream<ap_uint<PARALLEL_BYTE * 8> >& litStream,
                   hls::stream<ap_uint<LMO_WIDTH> >& litLenStream) {
    // align literals to input byte boundary as per literal length values
    const uint8_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    enum LiteralStates { READ_LITLEN = 0, WRITE_LITERAL };
align_block_lit_output:
    while (newBlockFlagStream.read()) {
        ap_uint<c_accRegWidth> inputWindow = 0;
        ap_uint<LMO_WIDTH> litLen;
        uint8_t bytesInAcc = 0;
        enum LiteralStates nextState = READ_LITLEN;
        bool readFlag = true;
        bool done = false;
        bool validLitLen = litlenValidStream.read();
        if (validLitLen == 0) continue;
        litLen = inLitLenStream.read();
        litLenStream << litLen;
        nextState = (LiteralStates)(litLen > 0);
        IntVectorStream_dt<PARALLEL_BYTE * 8, 1> inValue;

        inValue = inLitStream.read();
        inputWindow.range(c_streamWidth - 1, 0) = inValue.data[0];
        bytesInAcc += PARALLEL_BYTE;

    align_literals:
        while (!done) {
#pragma HLS PIPELINE II = 1
            uint8_t tmpCnt = 0;
            uint8_t shiftCnt = 0;
            uint8_t idxVal = bytesInAcc;
            if (nextState == READ_LITLEN) {
                validLitLen = litlenValidStream.read();
                if (validLitLen) {
                    litLen = inLitLenStream.read();
                    litLenStream << litLen;
                    done = false;
                } else {
                    done = true;
                }
            } else if (nextState == WRITE_LITERAL) {
                litStream << inputWindow.range(c_streamWidth - 1, 0);
                if (litLen >= PARALLEL_BYTE) {
                    shiftCnt = PARALLEL_BYTE;
                    tmpCnt = PARALLEL_BYTE;
                    litLen -= PARALLEL_BYTE;
                } else {
                    tmpCnt = litLen;
                    shiftCnt = litLen;
                    litLen = 0;
                }
            }
            idxVal -= tmpCnt;
            inputWindow >>= shiftCnt * 8;

            if (litLen) {
                nextState = WRITE_LITERAL;
                readFlag = ((idxVal < litLen) && (inValue.strobe));
            } else {
                nextState = READ_LITLEN;
                readFlag = false;
            }
            if (readFlag) {
                inValue = inLitStream.read();
                inputWindow.range((idxVal + PARALLEL_BYTE) * 8 - 1, idxVal * 8) = inValue.data[0];
                bytesInAcc += PARALLEL_BYTE - tmpCnt;
            } else {
                bytesInAcc -= tmpCnt;
            }
        }
        inValue = inLitStream.read();
    }
    litLenStream << 0;
}