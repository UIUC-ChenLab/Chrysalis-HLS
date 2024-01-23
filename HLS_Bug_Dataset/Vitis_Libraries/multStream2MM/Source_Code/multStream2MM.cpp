void multStream2MM(hls::stream<ap_uint<IN_DATAWIDTH> > inStream[NUM_BLOCK],
                   hls::stream<bool> inStreamEos[NUM_BLOCK],
                   hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                   const uint32_t output_idx[NUM_BLOCK],
                   ap_uint<GMEM_DATAWIDTH>* out,
                   uint32_t outSize[NUM_BLOCK]) {
    /**
     * @brief This module reads IN_DATAWIDTH data from stream based on the end
     * flag stream and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API.
     *
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam IN_DATAWIDTH width of input data bus
     * @tparam GMEM_DATAWIDTH width of output data bus
     * @tparam NUM_BLOCK number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param inSizeStream size flag for input stream
     * @param totalOutSizeStream size of output stream
     * @param output_size output size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV[NUM_BLOCK];
    hls::stream<uint16_t> outStreamVSize[NUM_BLOCK];
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
parallel_upsizer:
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        xf::compression::details::stream2mmUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(
            inStream[i], inStreamEos[i], outStreamV[i], outStreamVSize[i]);
    }
    xf::compression::details::multStream2mmSize<BURST_SIZE, GMEM_DATAWIDTH, NUM_BLOCK>(
        outStreamV, outStreamVSize, totalOutSizeStream, output_idx, out, outSize);
}

// Content of called function
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

// Content of called function
void stream2mmUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                      hls::stream<bool>& inStreamEos,
                      hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                      hls::stream<uint16_t>& outSizeStream) {
    /**
     * @brief This module reads IN_WIDTH data from stream until end of
     * stream happens and transfers OUT_WIDTH data into stream along with the
     * size of the chunk.
     *
     * @tparam IN_WIDTH width of input data bus
     * @tparam OUT_WIDTH width of output data bus
     * @tparam BURST_SIZE burst size
     *
     * @param inStream input stream
     * @param inStreamEos end flag for stream
     * @param outStream output stream
     * @param outSizeStream size stream for data stream
     */

    const int c_byteWidth = 8;
    const int c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    const int c_wordSize = OUT_WIDTH / c_byteWidth;
    const int c_size = BURST_SIZE * c_wordSize;

    ap_uint<OUT_WIDTH> outBuffer = 0;
    uint32_t byteIdx = 0;
    uint16_t sizeWrite = 0;
    ap_uint<IN_WIDTH> inValue = inStream.read();
stream_upsizer:
    for (bool eos_flag = inStreamEos.read(); eos_flag == false; eos_flag = inStreamEos.read()) {
#pragma HLS PIPELINE II = 1
        if (byteIdx == c_upsizeFactor) {
            outStream << outBuffer;
            sizeWrite++;
            if (sizeWrite == BURST_SIZE) {
                outSizeStream << c_size;
                sizeWrite = 0;
            }
            byteIdx = 0;
        }
        outBuffer.range((byteIdx + 1) * IN_WIDTH - 1, byteIdx * IN_WIDTH) = inValue;
        byteIdx++;
        inValue = inStream.read();
    }

    if (byteIdx) {
        outStream << outBuffer;
        sizeWrite++;
        outSizeStream << (sizeWrite * c_wordSize);
    }
    outSizeStream << 0;
}