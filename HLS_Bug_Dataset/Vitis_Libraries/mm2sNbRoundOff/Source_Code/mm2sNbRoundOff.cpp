void mm2sNbRoundOff(const ap_uint<DATAWIDTH>* in,
                    const uint32_t _input_idx[NUM_BLOCKS],
                    hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
                    const uint32_t _input_size[NUM_BLOCKS]) {
    /**
     * @brief This module is same as mm2sNb API but with an extra handling
     * rounding off the indexing to maximum buffer size for P2P decompression.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param in input memory address
     * @param _input_idx input index
     * @param outStream output stream
     * @param _input_size input stream size
     * @param max_buffer_size_in_bytes Maximum buffer size for indexing
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_idx[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<NUM_BLOCKS> pending;
    ap_uint<NUM_BLOCKS> is_full;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
#pragma HLS UNROLL
        read_idx[bIdx] = 0;
        write_idx[bIdx] = 0;
        read_size[bIdx] = 0;
        input_idx[bIdx] = _input_idx[bIdx];
        input_size[bIdx] = _input_size[bIdx] + (input_idx[bIdx] % c_wordSize);
        pending.range(bIdx, bIdx) = 1;
    }
    while (pending) {
        pending = 0;
        for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
            uint32_t pending_bytes = (input_size[bIdx] > read_size[bIdx]) ? (input_size[bIdx] - read_size[bIdx]) : 0;
            if ((pending_bytes) && (read_idx[bIdx] == write_idx[bIdx])) {
                uint32_t pending_words = (pending_bytes - 1) / c_wordSize + 1;
                uint32_t burst_size = (pending_words > BURST_SIZE) ? BURST_SIZE : pending_words;
                uint32_t mem_read_byte_idx = read_size[bIdx] + input_idx[bIdx];
                uint32_t mem_read_word_idx = 0;
                if (mem_read_byte_idx)
                    mem_read_word_idx = (mem_read_byte_idx % c_wordSize) ? (mem_read_byte_idx - 1) / c_wordSize
                                                                         : ((mem_read_byte_idx - 1) / c_wordSize + 1);
                else
                    mem_read_word_idx = 0;

            gmem_rd:
                for (uint32_t i = 0; i < burst_size; i++) {
#pragma HLS PIPELINE II = 1
                    local_buffer[bIdx][i] = in[mem_read_word_idx + i];
                }
                pending.range(bIdx, bIdx) = 1;
                read_idx[bIdx] = 0;
                write_idx[bIdx] = burst_size;
                read_size[bIdx] += burst_size * c_wordSize;
            }
        }
        ap_uint<NUM_BLOCKS> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
#pragma HLS UNROLL
                if (read_idx[bIdx] == write_idx[bIdx]) {
                    terminate_all.range(bIdx, bIdx) = 0;
                    if (read_size[bIdx] < input_size[bIdx]) {
                        terminate = 1;
                    }
                } else {
                    terminate_all.range(bIdx, bIdx) = 1;
                    pending.range(bIdx, bIdx) = 1;
                }
            }
        }
    }
}

// Content of called function
void mm2sNb(const ap_uint<DATAWIDTH>* in,
            const uint32_t _input_idx[NUM_BLOCKS],
            hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
            const uint32_t _input_size[NUM_BLOCKS]) {
    /**
     * @brief This module reads 512bit data from memory interface and
     * writes to the stream. Writing to the multiple data streams is
     * non-blocking call which is done using is_full() API
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param in input memory address
     * @param _input_idx input index
     * @param outStream output stream
     * @param _input_size input stream size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_idx[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<NUM_BLOCKS> pending;
    ap_uint<NUM_BLOCKS> is_full;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
#pragma HLS UNROLL
        read_idx[bIdx] = 0;
        write_idx[bIdx] = 0;
        read_size[bIdx] = 0;
        input_idx[bIdx] = _input_idx[bIdx];
        input_size[bIdx] = _input_size[bIdx];
        pending.range(bIdx, bIdx) = 1;
    }
    while (pending) {
        pending = 0;
        for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
            uint32_t pending_bytes = (input_size[bIdx] > read_size[bIdx]) ? (input_size[bIdx] - read_size[bIdx]) : 0;
            if ((pending_bytes) && (read_idx[bIdx] == write_idx[bIdx])) {
                uint32_t pending_words = (pending_bytes - 1) / c_wordSize + 1;
                uint32_t burst_size = (pending_words > BURST_SIZE) ? BURST_SIZE : pending_words;
                uint32_t mem_read_byte_idx = read_size[bIdx] + input_idx[bIdx];
                uint32_t mem_read_word_idx = (mem_read_byte_idx) ? ((mem_read_byte_idx - 1) / c_wordSize + 1) : 0;
            gmem_rd:
                for (uint32_t i = 0; i < burst_size; i++) {
#pragma HLS PIPELINE II = 1
                    local_buffer[bIdx][i] = in[mem_read_word_idx + i];
                }
                pending.range(bIdx, bIdx) = 1;
                read_idx[bIdx] = 0;
                write_idx[bIdx] = burst_size;
                read_size[bIdx] += burst_size * c_wordSize;
            }
        }
        ap_uint<NUM_BLOCKS> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
#pragma HLS UNROLL
                if (read_idx[bIdx] == write_idx[bIdx]) {
                    terminate_all.range(bIdx, bIdx) = 0;
                    if (read_size[bIdx] < input_size[bIdx]) {
                        terminate = 1;
                    }
                } else {
                    terminate_all.range(bIdx, bIdx) = 1;
                    pending.range(bIdx, bIdx) = 1;
                }
            }
        }
    }
}