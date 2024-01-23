void streamUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                   hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                   SIZE_DT original_size) {
    /**
     * @brief This module reads IN_WIDTH from the input stream and accumulate
     * the consecutive reads until OUT_WIDTH and writes the OUT_WIDTH data to
     * output stream
     *
     * @tparam SIZE_DT stream size class instance
     * @tparam IN_WIDTH input data width
     * @tparam OUT_WIDTH output data width
     *
     * @param inStream input stream
     * @param outStream output stream
     * @param original_size original stream size
     */

    if (original_size == 0) return;

    uint8_t paralle_byte = IN_WIDTH / 8;
    ap_uint<OUT_WIDTH> shift_register;
    uint8_t factor = OUT_WIDTH / IN_WIDTH;
    original_size = (original_size - 1) / paralle_byte + 1;
    uint32_t withAppendedDataSize = (((original_size - 1) / factor) + 1) * factor;

    for (uint32_t i = 0; i < withAppendedDataSize; i++) {
#pragma HLS PIPELINE II = 1
        if (i != 0 && i % factor == 0) {
            outStream << shift_register;
            shift_register = 0;
        }
        if (i < original_size) {
            shift_register.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = inStream.read();
        } else {
            shift_register.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = 0;
        }
        if ((i + 1) % factor != 0) shift_register >>= IN_WIDTH;
    }
    // write last data to stream
    outStream << shift_register;
}