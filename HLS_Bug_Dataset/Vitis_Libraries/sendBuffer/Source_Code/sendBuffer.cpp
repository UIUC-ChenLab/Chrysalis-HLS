void sendBuffer(hls::stream<IntVectorStream_dt<D_WIDTH, 1> >& inStream,
                hls::stream<ap_uint<D_WIDTH> >& outStream,
                hls::stream<ap_uint<17> >& outSize) {
    bool last = false;

buffer_top:
    while (!last) {
        last = true;
        ap_uint<17> sizeCntr = 0;
        auto inVal = inStream.read();
        bool loop_continue = (inVal.strobe != 0);
    buffer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            loop_continue = (inVal.strobe != 0);
            if (!loop_continue) break;
            outStream << inVal.data[0];
            if (inVal.strobe != 0) {
                inVal = inStream.read();
                sizeCntr++;
            }
        }
        // write out size of up-sized data to terminate the block
        outSize << sizeCntr;
    }
}