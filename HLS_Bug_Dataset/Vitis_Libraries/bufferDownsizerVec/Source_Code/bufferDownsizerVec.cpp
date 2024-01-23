void bufferDownsizerVec(hls::stream<ap_uint<IN_DATAWIDTH + SIZE_DWIDTH> >& inStream,
                        hls::stream<IntVectorStream_dt<8, OUT_DATAWIDTH / 8> >& outStream) {
    constexpr uint16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr uint8_t c_outWord = OUT_DATAWIDTH / 8;
    constexpr uint8_t c_outDataHigh = OUT_DATAWIDTH + SIZE_DWIDTH - 1;
    IntVectorStream_dt<8, c_outWord> outVal;

downsizer_top:
    while (1) {
        auto inVal = inStream.read();
        // proceed further if valid size
        ap_uint<SIZE_DWIDTH> inSize = inVal.range(SIZE_DWIDTH - 1, 0);
        if (inSize == 0) break;
    downsizer_assign:
        while (inSize > 0) {
#pragma HLS PIPELINE II = 1
            ap_uint<OUT_DATAWIDTH> outReg = inVal.range(c_outDataHigh, SIZE_DWIDTH);
            inVal >>= OUT_DATAWIDTH;
            outVal.strobe = ((inSize < c_outWord) ? (uint8_t)inSize : c_outWord);
            for (uint8_t i = 0; i < c_outWord; ++i) {
#pragma HLS UNROLL
                outVal.data[i] = outReg.range((i * 8) + 7, i * 8);
            }
            outStream << outVal;
            inSize -= outVal.strobe;
            if (inSize == 0) {
                inVal = inStream.read();
                inSize = inVal.range(SIZE_DWIDTH - 1, 0);
            }
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}