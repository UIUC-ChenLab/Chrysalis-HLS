void mm2sSimple(const ap_uint<DATAWIDTH>* in,
                hls::stream<ap_uint<DATAWIDTH> >& outstream,
                hls::stream<uint32_t>& sizeStream,
                hls::stream<uint32_t>& inSize) {
    /**
     * @brief Read data from DATAWIDTH wide axi memory interface and
     *        write to stream.
     *
     * @tparam DATAWIDTH    width of data bus
     *
     * @param in            pointer to input memory
     * @param outstream     output stream
     * @param inSize        size of the data
     * @param sizeStream    o/p size of the data
     *
     */
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    uint32_t inputSize = inSize.read();
    sizeStream << inputSize;
    uint32_t inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

    int allignedwidth = inSize_gmemwidth / BURST_SIZE;
    allignedwidth = ((inSize_gmemwidth - allignedwidth) > 0) ? allignedwidth + 1 : allignedwidth;

    int i = 0;
    ap_uint<DATAWIDTH> temp;
mm2s_simple:
    for (; i < allignedwidth * BURST_SIZE; i += BURST_SIZE) {
    burst_transfer:
        for (uint32_t j = 0; j < BURST_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            temp = in[i + j];
            if ((i + j) < inSize_gmemwidth) outstream << temp;
        }
    }
}