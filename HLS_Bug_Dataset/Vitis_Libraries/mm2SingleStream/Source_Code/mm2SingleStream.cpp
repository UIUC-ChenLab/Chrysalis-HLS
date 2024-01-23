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