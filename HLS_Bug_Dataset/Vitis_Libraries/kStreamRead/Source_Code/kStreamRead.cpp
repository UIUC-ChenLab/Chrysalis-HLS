void kStreamRead(hls::stream<ap_axiu<DATAWIDTH, 0, 0, 0> >& inKStream,
                 hls::stream<ap_uint<DATAWIDTH> >& readStream,
                 uint32_t input_size) {
    /**
     * @brief Read N-bit wide data from internal streams output by compression modules
     *        and write to output axi stream. N is the template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH    data width of the kernel stream
     *
     * @param inKStream     input axi kernel stream
     * @param readStream    internal hls stream to be read for processing
     *
     */
    ap_axiu<DATAWIDTH, 0, 0, 0> tmp;
    uint8_t factor = DATAWIDTH / 8;
    uint32_t sCnt = 1 + ((input_size - 1) / factor);

    for (int i = 0; i < sCnt; i++) {
#pragma HLS PIPELINE II = 1
        tmp = inKStream.read();
        readStream << tmp.data;
    }
}