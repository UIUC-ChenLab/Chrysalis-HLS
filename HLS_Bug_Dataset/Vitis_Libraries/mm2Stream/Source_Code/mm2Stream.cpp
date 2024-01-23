void mm2Stream(const ap_uint<GMEM_DATAWIDTH>* in,
               hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
               const uint32_t _input_size) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams in 8-bit chunks. Writing to the multiple data streams is
     * non-blocking call which is done using is_full() API
     *
     * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
     * @tparam BURST_SIZE burst size of the data transfers
     *
     *
     * @param in input memory address
     * @param outStream output stream
     * @param _input_size input stream size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    // Array of Streams used as internal buffer.
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV;
    hls::stream<uint16_t> outStreamVSize;
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
    xf::compression::details::mm2SingleStream<GMEM_DATAWIDTH, BURST_SIZE>(in, outStreamV, outStreamVSize, _input_size);
    xf::compression::details::mm2StreamDownSizer<GMEM_DATAWIDTH, OUT_DATAWIDTH>(outStreamV, outStreamVSize, outStream);
}

// Content of called function
void mm2SingleStream(const ap_uint<DATAWIDTH>* in,
                     hls::stream<ap_uint<DATAWIDTH> >& outStream,
                     hls::stream<uint16_t>& outSizeStream,
                     const uint32_t _input_size) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams and output size streams
     *
     * @tparam DATAWIDTH input width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     *
     * @param in input memory address
     * @param outStream output stream
     * @param outSizeStream output size stream
     * @param _input_size input stream size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const uint32_t c_burstSize = BURST_SIZE * c_wordSize;

    uint32_t read_idx = 0;
    uint32_t read_size = 0;
    uint32_t input_size = _input_size;

mm2StreamSimple:
    for (uint32_t idx = 0; idx < input_size; idx += c_burstSize) {
        uint32_t pendingBytes = (input_size > read_size) ? (input_size - read_size) : 0;
        uint32_t sizeWrite = 0;
        uint32_t pendingWords = (pendingBytes - 1) / c_wordSize + 1;
        uint32_t burstSize = (pendingWords > BURST_SIZE) ? BURST_SIZE : pendingWords;
        sizeWrite = burstSize * c_wordSize;
        if (read_size + sizeWrite < input_size) {
            outSizeStream << sizeWrite;
            read_size += sizeWrite;
        } else {
            outSizeStream << (input_size - read_size);
            read_size = input_size;
        }
    gmem_read:
        for (uint32_t midx = 0; midx < burstSize; midx++) {
#pragma HLS PIPELINE II = 1
            outStream << in[read_idx + midx];
        }
        read_idx += burstSize;
    }
    outSizeStream << 0;
}

// Content of called function
void mm2StreamDownSizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
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
            if (idx == 0) {
                inBuffer = inStream.read();
            } else {
                inBuffer >>= OUT_DATAWIDTH;
            }
            outStream << inBuffer.range(OUT_DATAWIDTH - 1, 0);
        }
    }
}