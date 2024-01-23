void hlsStream2axiStreamFixedSize(hls::stream<ap_uint<STREAMDWIDTH> >& hlsInStream,
                                  hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& outputAxiStream,
                                  uint32_t originalSize) {
    /**
     * @brief Read data from internal hls stream and write to output axi stream for the given size.
     *
     * @param hlsInStream       internal hls stream
     * @param outputAxisStream  output axi stream
     * @param originalSize      output data size to be written to output stream
     *
     */
    ap_uint<STREAMDWIDTH> temp;
    for (int i = 0; i < originalSize - 1; i++) {
        temp = hlsInStream.read();
        qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
        tOut.set_data(temp);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    }
    // last byte
    temp = hlsInStream.read();
    qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
    tOut.set_data(temp);
    tOut.set_last(1);
    tOut.set_keep(-1);
    outputAxiStream.write(tOut);
}