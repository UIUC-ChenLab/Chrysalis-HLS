void bufferDownsizer(hls::stream<ap_uint<IN_DATAWIDTH + SIZE_DWIDTH> >& inStream,
                     hls::stream<IntVectorStream_dt<OUT_DATAWIDTH, 1> >& outStream) {
    constexpr int16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr int16_t c_outWord = OUT_DATAWIDTH / 8;
    IntVectorStream_dt<OUT_DATAWIDTH, 1> outVal;

downsizer_top:
    while (1) {
        ap_uint<SIZE_DWIDTH> dsize = 0;
        auto inVal = inStream.read();
        // proceed further if valid size
        ap_uint<SIZE_DWIDTH> inSize = inVal.range(SIZE_DWIDTH - 1, 0);
        if (inSize == 0) break;
        auto outSizeV = ((inSize - 1) / c_outWord) + 1;
        outVal.strobe = 1;
    downsizer_assign:
        while (inSize > 0) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = inVal.range(OUT_DATAWIDTH + SIZE_DWIDTH - 1, SIZE_DWIDTH);
            inVal >>= OUT_DATAWIDTH;
            outStream << outVal;
            dsize += c_outWord;
            if (dsize == outSizeV) {
                inVal = inStream.read();
                inSize = inVal.range(SIZE_DWIDTH - 1, 0);
                dsize = 0;
                outSizeV = ((inSize - 1) / c_outWord) + 1;
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