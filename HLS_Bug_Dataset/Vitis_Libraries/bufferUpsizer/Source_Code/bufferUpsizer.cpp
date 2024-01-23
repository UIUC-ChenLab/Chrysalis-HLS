void bufferUpsizer(hls::stream<IntVectorStream_dt<IN_WIDTH, 1> >& inStream,
                   hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                   hls::stream<ap_uint<17> >& outSize) {
    constexpr uint8_t c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr uint8_t c_inBytes = IN_WIDTH / 8;
    constexpr uint8_t c_outWidth = OUT_WIDTH;
    ap_uint<c_outWidth> outVal;
    bool last = false;

buffer_upsizer_top:
    while (!last) {
        last = true;
        int8_t byteIdx = 0;
        ap_uint<17> sizeCntr = 0;
        auto inVal = inStream.read();
        bool loop_continue = (inVal.strobe != 0);
    buffer_upsizer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            if (byteIdx == c_upsizeFactor) {
                // append valid bytes count to output packet
                outStream << outVal;
                byteIdx = 0;
                loop_continue = (inVal.strobe != 0);
            }
            outVal >>= IN_WIDTH;
            outVal.range(c_outWidth - 1, c_outWidth - IN_WIDTH) = inVal.data[0];
            ++byteIdx;
            if (inVal.strobe != 0) {
                inVal = inStream.read();
                sizeCntr += c_inBytes;
            }
        }
        // write out size of up-sized data to terminate the block
        outSize << sizeCntr;
    }
}