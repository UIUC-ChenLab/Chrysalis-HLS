void mm2multStreamSize(const ap_uint<IN_DATAWIDTH>* in,
                       const uint32_t input_idx[NUM_BLOCKS],
                       hls::stream<ap_uint<OUT_DATAWIDTH> > outStream[NUM_BLOCKS],
                       const uint32_t _input_size[NUM_BLOCKS]) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams in 8-bit chunks. Writing to the multiple data streams is
     * non-blocking call which is done using full() API
     *
     * @tparam NUM_BLOCKS number of parallel blocks
     * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
     * @tparam BURST_SIZE burst size of the data transfers
     *
     *
     * @param in input memory address
     * @param input_idx input index
     * @param outStream output stream
     * @param _input_size input size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    // Array of Streams used as internal buffer.
    hls::stream<ap_uint<IN_DATAWIDTH> > outStreamV[NUM_BLOCKS];
    hls::stream<uint16_t> outStreamVSize[NUM_BLOCKS];
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 3
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
    xf::compression::details::mm2multStreamSimple<NUM_BLOCKS, IN_DATAWIDTH, BURST_SIZE>(in, outStreamV, outStreamVSize,
                                                                                        input_idx, _input_size);
downsizer:
    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        xf::compression::details::mm2multStreamDownSizer<IN_DATAWIDTH, OUT_DATAWIDTH>(
            outStreamV[vid], outStreamVSize[vid], outStream[vid]);
    }
}

// Content of called function
void mm2multStreamDownSizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                            hls::stream<uint16_t>& inSizeStream,
                            hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream) {
    /**
     * @brief This module reads 512-bit data from stream interface and
     * writes to the output stream in 8-bit chunks using the size stream.
     *
     * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
     *
     * @param inStream input stream
     * @param inSizeStream input size stream
     * @param outStream output stream
     */

    const int c_byteWidth = 8;
    const int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    const int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    const int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;

downsizer_top:
    for (uint16_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint16_t outSizeV = (inSize - 1) / c_outWord + 1;
    downsizer_assign:
        for (uint16_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
            int idx = itr % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<OUT_DATAWIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_DATAWIDTH - 1, idx * OUT_DATAWIDTH);
            outStream << tmpValue;
        }
    }
}

// Content of called function
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