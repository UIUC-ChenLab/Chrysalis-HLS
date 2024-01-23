void mm2multStreamSimple(const ap_uint<DATAWIDTH>* in,
                         hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
                         hls::stream<uint16_t> outSizeStream[NUM_BLOCKS],
                         const uint32_t input_idx[NUM_BLOCKS],
                         const uint32_t _input_size[NUM_BLOCKS]) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams and output size streams
     *
     * @tparam DATAWIDTH input width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam NUM_BLOCKS number of parallel blocks
     *
     * @param in input memory address
     * @param input_idx input index
     * @param outStream output stream
     * @param outSizeStream output size stream
     * @param _input_size input stream size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;

    ap_uint<NUM_BLOCKS> is_pending;
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete

    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        read_idx[vid] = input_idx[vid] / c_wordSize;
        input_size[vid] = _input_size[vid];
        read_size[vid] = 0;
        is_pending.range(vid, vid) = 1;
    }

    while (is_pending) {
    parallel_ops:
        for (uint32_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS PIPELINE off
            bool isFull = (outSizeStream[vid]).full();
            uint32_t pendingBytes = (input_size[vid] > read_size[vid]) ? (input_size[vid] - read_size[vid]) : 0;
            is_pending.range(vid, vid) = (pendingBytes > 0) ? 1 : 0;
            uint32_t sizeWrite = 0;
            if (pendingBytes && !isFull) {
                uint32_t pendingWords = (pendingBytes - 1) / c_wordSize + 1;
                uint32_t burstSize = (pendingWords > BURST_SIZE) ? BURST_SIZE : pendingWords;
                sizeWrite = burstSize * c_wordSize;
                uint32_t rIdx = read_idx[vid];
            gmem_read:
                for (uint32_t midx = 0; midx < burstSize; midx++) {
                    outStream[vid] << in[rIdx + midx];
                }
                read_idx[vid] += burstSize;
                if (read_size[vid] + sizeWrite < input_size[vid]) {
                    outSizeStream[vid] << sizeWrite;
                    read_size[vid] += sizeWrite;
                } else {
                    outSizeStream[vid] << (input_size[vid] - read_size[vid]);
                    read_size[vid] = input_size[vid];
                }
            }
        }
    }

size_init:
    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        outSizeStream[vid] << 0;
    }
}