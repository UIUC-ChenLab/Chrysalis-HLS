void bufferDownsizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                     hls::stream<IntVectorStream_dt<OUT_DATAWIDTH, 1> >& outStream,
                     hls::stream<ap_uint<17> >& inputSize) {
    constexpr int16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr int16_t c_outWord = OUT_DATAWIDTH / 8;
    IntVectorStream_dt<OUT_DATAWIDTH, 1> outVal;

downsizer_top:
    while (1) {
        ap_uint<17> inSize = inputSize.read();
        ap_uint<IN_DATAWIDTH> inVal = 0;
        ap_uint<17> cntr = 0;
        // proceed further if valid size
        if (inSize == 0) break;
        auto outSizeV = ((inSize - 1) / c_outWord) + 1;
        outVal.strobe = 1;
    downsizer_assign:
        for (auto i = 0; i < outSizeV; i += c_outWord) {
#pragma HLS PIPELINE II = 1
            if (cntr % c_factor == 0) inVal = inStream.read();
            outVal.data[0] = inVal.range(OUT_DATAWIDTH - 1, 0);
            inVal >>= OUT_DATAWIDTH;
            outStream << outVal;
            cntr++;
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}