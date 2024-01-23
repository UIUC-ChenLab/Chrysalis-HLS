void axis2hlsStreamFixedSize(hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& inputAxiStream,
                             hls::stream<ap_uint<STREAMDWIDTH> >& inputStream,
                             uint32_t inputSize) {
    /**
     * @brief Read data from axi stream and write to internal hls stream.
     *
     * @param inputAxisStream   incoming axi stream
     * @param inputStream       output hls stream
     * @param inputSize         size of the data coming from input axi stream
     *
     */
    for (int i = 0; i < inputSize; i++) {
#pragma HLS PIPELINE II = 1
        qdma_axis<STREAMDWIDTH, 0, 0, 0> t1 = inputAxiStream.read();
        ap_uint<STREAMDWIDTH> tmpOut;
        tmpOut = t1.get_data();
        inputStream << tmpOut;
    }
}