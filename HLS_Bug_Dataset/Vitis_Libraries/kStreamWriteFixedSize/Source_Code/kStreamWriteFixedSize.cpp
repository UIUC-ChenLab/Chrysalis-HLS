void kStreamWriteFixedSize(hls::stream<ap_axiu<DATAWIDTH, 0, 0, 0> >& outKStream,
                           hls::stream<ap_uint<DATAWIDTH> >& outDataStream,
                           uint32_t outputSize) {
    /**
     * @brief Read N-bit wide data from internal hls streams and write to axi
     *        kernel stream for the given size. N is the template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH    data width of the kernel stream
     *
     * @param outKStream    output kernel stream
     * @param outDataStream output data stream from internal modules
     * @param dataSize      size of data in streams
     *
     */
    uint32_t outSize = 0;
    ap_uint<DATAWIDTH> tmp;
    ap_axiu<DATAWIDTH, 0, 0, 0> t1;

    uint8_t factor = DATAWIDTH / 8;
    uint32_t sCnt = 1 + ((outputSize - 1) / factor);

    for (uint32_t i = 0; i < sCnt - 1; i++) {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        t1.data = tmp;
        t1.last = false;
        outKStream.write(t1);
    }
    // last data packet
    tmp = outDataStream.read();
    t1.data = tmp;
    t1.last = true;
    outKStream.write(t1);
}