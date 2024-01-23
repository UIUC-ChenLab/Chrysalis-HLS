void mm2sSimple(const ap_uint<DATAWIDTH>* in, hls::stream<ap_uint<DATAWIDTH> >& outstream, uint32_t inputSize) {
    /**
     * @brief Read data from 512-bit wide axi memory interface and
     *        write to stream.
     *
     * @tparam DATAWIDTH    width of data bus
     *
     * @param in            pointer to input memory
     * @param outstream     output stream
     * @param inputSize     size of the data
     *
     */
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

mm2s_simple:
    for (int i = 0; i < inSize_gmemwidth; i++) {
#pragma HLS PIPELINE II = 1
        outstream << in[i];
    }
}