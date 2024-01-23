void streamDataK2dmFixedSize(hls::stream<ap_uint<STREAMDWIDTH> >& out,
                             hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& dmOutStream,
                             uint32_t dataSize) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out           output hls stream
     * @param dmOutStream   input kernel axi stream to be read
     * @param dataSize      size of data in streams
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<STREAMDWIDTH, 0, 0, 0> dataout;
    for (uint32_t i = 0; i < dataSize; i++) {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        out << dataout.data;
    }
}