void streamDownSizerSize(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                         hls::stream<ap_uint<SIZE_DWIDTH> >& dataSizeStream,
                         hls::stream<IntVectorStream_dt<OUT_DATAWIDTH, 1> >& outStream) {
    constexpr int16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr int16_t c_outWord = OUT_DATAWIDTH / 8;
    ap_uint<IN_DATAWIDTH> inVal;
    IntVectorStream_dt<OUT_DATAWIDTH, 1> outVal;
    ap_uint<SIZE_DWIDTH> inSize = 0;

downsizer_top:
    for (auto inSize = dataSizeStream.read(); inSize > 0; inSize = dataSizeStream.read()) {
        auto outSizeV = ((inSize - 1) / c_outWord) + 1;
        outVal.strobe = 1;
    downsizer_assign:
        for (ap_uint<SIZE_DWIDTH> dsize = 0; dsize < outSizeV; ++dsize) {
#pragma HLS PIPELINE II = 1
            auto idx = dsize % c_factor;
            if (idx == 0) {
                inVal = inStream.read();
            }
            outVal.data[0] = inVal.range(OUT_DATAWIDTH - 1, 0);
            inVal >>= OUT_DATAWIDTH;
            outStream << outVal;
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}