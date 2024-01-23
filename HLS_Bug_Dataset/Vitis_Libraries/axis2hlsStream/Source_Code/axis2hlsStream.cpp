void axis2hlsStream(hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& inAxiStream,
                    hls::stream<ap_uint<STREAMDWIDTH> >& outStream) {
    /**
     * @brief Read N-bit wide data from internal hls streams
     *        and write to output axi stream. N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param inAxiStream   input kernel axi stream
     * @param outStream     output hls stream
     *
     */
    while (true) {
#pragma HLS PIPELINE II = 1
        qdma_axis<STREAMDWIDTH, 0, 0, 0> t1 = inAxiStream.read();
        ap_uint<STREAMDWIDTH> tmp = t1.get_data();
        outStream << tmp;
        if (t1.get_last()) {
            break;
        }
    }
}