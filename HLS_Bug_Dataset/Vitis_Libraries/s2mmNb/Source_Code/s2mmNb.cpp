void s2mmNb(ap_uint<DATAWIDTH>* out,
            const uint32_t output_idx[NUM_BLOCKS],
            hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCKS],
            const STREAM_SIZE_DT input_size[NUM_BLOCKS]) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR. Reading data from
     * multiple data streams is non-blocking which is done using empty() API.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param input_size input size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = c_wordSize * BURST_SIZE;
    uint32_t read_size[NUM_BLOCKS];
    uint32_t write_size[NUM_BLOCKS];
    uint32_t burst_size[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete
    ap_uint<NUM_BLOCKS> end_of_stream = 0;
    ap_uint<NUM_BLOCKS> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM

    // printme("%s:Started\n", __FUNCTION__);
    for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        read_size[i] = 0;
        write_size[i] = 0;
        write_idx[i] = 0;
        // printme("%s:Indx=%d out_idx=%d\n",__FUNCTION__,i , output_idx[i]);
    }
    bool done = false;
    uint32_t loc = 0;
    uint32_t remaining_data = 0;
    while (is_pending != 0) {
        done = false;
        for (int i = 0; (is_pending != 0) && (done == 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                burst_size[pb] = c_maxBurstSize;
                if (((input_size[pb] - write_size[pb]) < burst_size[pb])) {
                    burst_size[pb] = (input_size[pb] > write_size[pb]) ? (input_size[pb] - write_size[pb]) : 0;
                }
                if (((read_size[pb] - write_size[pb]) < burst_size[pb]) && (input_size[pb] > read_size[pb])) {
                    bool is_empty = inStream[pb].empty();
                    if (!is_empty) {
                        local_buffer[pb][write_idx[pb]] = inStream[pb].read();
                        write_idx[pb] += 1;
                        read_size[pb] += c_wordSize;
                        is_pending.range(pb, pb) = true;
                    } else {
                        is_pending.range(pb, pb) = false;
                    }
                } else {
                    if (burst_size[pb]) done = true;
                }
            }
        }

        for (int i = 0; i < NUM_BLOCKS; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] + write_size[i];
                uint32_t base_idx = base_addr / c_wordSize;
                uint32_t burst_size_in_words = (burst_size[i]) ? ((burst_size[i] - 1) / c_wordSize + 1) : 0;

                if (burst_size_in_words > 0) {
                    for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                        out[base_idx + j] = local_buffer[i][j];
                    }
                }
                write_size[i] += burst_size[i];
                write_idx[i] = 0;
            }
        }
        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (done == true && (write_size[i] >= input_size[i])) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }
}