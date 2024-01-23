void streamDataDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                    hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm,
                    uint32_t inputSize) {
    /**
     * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
     *        N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param in            input hls stream
     * @param inStream_dm   output kernel stream
     * @param inputSize     size of data in to be transferred
     *
     */
    // read data from input hls to input stream for decompression kernel
    uint32_t itrLim = 1 + (inputSize - 1) / (STREAMDWIDTH / 8);
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<STREAMDWIDTH> temp = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> dataIn;
        dataIn.data = temp; // kernel to kernel data transfer
        inStream_dm.write(dataIn);
    }
}