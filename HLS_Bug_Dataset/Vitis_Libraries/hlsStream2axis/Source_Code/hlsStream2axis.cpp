void hlsStream2axis(hls::stream<ap_uint<STREAMDWIDTH> >& outputStream,
                    hls::stream<bool>& outStreamEos,
                    hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& outputAxiStream,
                    hls::stream<uint32_t>& outStreamSize,
                    hls::stream<qdma_axis<32, 0, 0, 0> >& outAxiStreamSize) {
    /**
     * @brief Read data from hls stream till the end of stream is indicated and write this data to axi stream.
     *        The total size of data is written 32-bit wide axi stream.
     *
     * @param outputStream      internal output hls stream
     * @param outStreamEos      stream to specify the end of stream
     * @param outputAxisStream  output stream going to axi
     * @param outStreamSize     size of the data coming to output from stream
     * @param outAxiStreamSize  size of the data to go through output axi stream
     *
     */
    ap_uint<STREAMDWIDTH> temp;
    bool flag;
    do {
        temp = outputStream.read();
        flag = outStreamEos.read();
        qdma_axis<STREAMDWIDTH, 0, 0, 0> tOut;
        tOut.set_data(temp);
        tOut.set_last(flag);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    } while (!flag);
    uint32_t outSize = outStreamSize.read();
    qdma_axis<32, 0, 0, 0> tOutSize;
    tOutSize.set_data(outSize);
    tOutSize.set_last(1);
    tOutSize.set_keep(-1);
    outAxiStreamSize.write(tOutSize);
}