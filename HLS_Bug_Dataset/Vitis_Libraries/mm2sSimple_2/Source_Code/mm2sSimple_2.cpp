void mm2sSimple(const ap_uint<DATAWIDTH>* in, hls::stream<ap_uint<DATAWIDTH> >& outstream, uint32_t inputSize) {
    /**
     * @brief Read data from DATAWIDTH wide axi memory interface and
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

    int allignedwidth = inSize_gmemwidth / BURST_SIZE;
    allignedwidth = ((inSize_gmemwidth - allignedwidth) > 0) ? allignedwidth + 1 : allignedwidth;

    int i = 0;
    ap_uint<DATAWIDTH> temp;
mm2s_simple:
    for (; i < allignedwidth * BURST_SIZE; i += BURST_SIZE) {
        for (uint32_t j = 0; j < BURST_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            temp = in[i + j];
            if ((i + j) < inSize_gmemwidth) outstream << temp;
        }
    }
}