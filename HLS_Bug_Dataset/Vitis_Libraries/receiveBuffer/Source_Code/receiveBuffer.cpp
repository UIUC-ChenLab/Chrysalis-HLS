void receiveBuffer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                   hls::stream<IntVectorStream_dt<IN_DATAWIDTH, 1> >& outStream,
                   hls::stream<ap_uint<17> >& inputSize) {
    IntVectorStream_dt<IN_DATAWIDTH, 1> outVal;

buffer_top:
    while (1) {
        ap_uint<17> inSize = inputSize.read();
        ap_uint<IN_DATAWIDTH> inVal = 0;
        // proceed further if valid size
        if (inSize == 0) break;
        auto outSizeV = inSize;
        outVal.strobe = 1;
    buffer_assign:
        for (auto i = 0; i < outSizeV; i++) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            outVal.data[0] = inVal;
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