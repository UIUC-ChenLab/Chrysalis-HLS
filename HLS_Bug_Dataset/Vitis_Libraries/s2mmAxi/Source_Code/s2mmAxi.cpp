void s2mmAxi(ap_uint<DATAWIDTH>* out,
             hls::stream<ap_uint<DATAWIDTH + PARALLEL_BYTES> >& inStream,
             uint32_t* outputSize) {
    /**
     * @brief This module reads data from AXI stream and
     * writes the data to DDR.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @param out output memory address
     * @param inStream input stream
     * @param outSize output data size
     */
    constexpr int c_parallelByte = DATAWIDTH / 8;
    ap_uint<DATAWIDTH + PARALLEL_BYTES> inData = inStream.read();
    uint32_t outSize = 0;
    uint32_t outIdx = 0;
    bool last = inData.range(PARALLEL_BYTES - 1, 0) == 0;

axi2Hls:
    while (!last) {
        for (auto i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
            out[outIdx + i] = inData.range(DATAWIDTH + PARALLEL_BYTES - 1, PARALLEL_BYTES);
            if (!last) {
                outSize += (32 - __builtin_clz(inData.range(PARALLEL_BYTES - 1, 0)));
                inData = inStream.read();
                last = inData.range(PARALLEL_BYTES - 1, 0) == 0;
            }
        }
        outIdx += BURST_SIZE;
    }

    outputSize[0] = outSize;
}