void multStream2mmSize(hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCK],
                       hls::stream<uint16_t> inSizeStream[NUM_BLOCK],
                       hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                       const uint32_t output_idx[NUM_BLOCK],
                       ap_uint<DATAWIDTH>* out,
                       uint32_t outSize[NUM_BLOCK]) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on the size
     * stream and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API.
     *
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCK number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param inSizeStream size flag for input stream
     * @param totalOutSizeStream size of output stream
     * @param output_size output size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;

    uint32_t write_size[NUM_BLOCK];
    uint32_t base_addr[NUM_BLOCK];
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = base_addr dim = 0 complete
    ap_uint<NUM_BLOCK> is_pending;

    for (int vid = 0; vid < NUM_BLOCK; vid++) {
#pragma HLS UNROLL
        write_size[vid] = 0;
        base_addr[vid] = output_idx[vid] / c_wordSize;
        is_pending.range(vid, vid) = 1;
    }

    while (is_pending) {
        for (int i = 0; i < NUM_BLOCK; i++) {
            uint32_t readSizeBytes = 0;
            if (!inSizeStream[i].empty()) {
                readSizeBytes = inSizeStream[i].read();
                is_pending.range(i, i) = (readSizeBytes > 0) ? 1 : 0;
            }
            if (readSizeBytes > 0) {
                uint32_t readSize = (readSizeBytes - 1) / c_wordSize + 1;
                uint32_t base_idx = base_addr[i] + write_size[i];
            gmem_write:
                for (int j = 0; j < readSize; j++) {
#pragma HLS PIPELINE II = 1
                    out[base_idx + j] = inStream[i].read();
                }
                write_size[i] += readSize;
            }
        }
    }

    for (uint8_t pb = 0; pb < NUM_BLOCK; pb++) {
#pragma HLS PIPELINE II = 1
        outSize[pb] = totalOutSizeStream[pb].read();
    }
}