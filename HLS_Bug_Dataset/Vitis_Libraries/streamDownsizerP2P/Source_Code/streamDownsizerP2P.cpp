void streamDownsizerP2P(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                        hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                        SIZE_DT input_size,
                        SIZE_DT input_start_idx) {
    /**
     * @brief This module reads the IN_WIDTH size from the data stream
     * and downsizes the data to OUT_WIDTH size and writes to output stream
     *
     * @tparam SIZE_DT data size
     * @tparam IN_WIDTH input width
     * @tparam OUT_WIDTH output width
     *
     * @param inStream input stream
     * @param outStream output stream
     * @param input_size input size
     * @param input_start_idx input starting index
     */
    const int c_byteWidth = 8;
    const int c_inputWord = IN_WIDTH / c_byteWidth;
    const int c_outWord = OUT_WIDTH / c_byteWidth;
    uint32_t sizeOutputV = (input_size - 1) / c_outWord + 1;
    int factor = c_inputWord / c_outWord;
    ap_uint<IN_WIDTH> inBuffer = 0;
    int offset = input_start_idx % c_inputWord;
convInWidthtoV:
    for (int i = offset; i < (sizeOutputV + offset); i++) {
#pragma HLS PIPELINE II = 1
        int idx = i % factor;
        if (idx == 0 || i == offset) inBuffer = inStream.read();
        ap_uint<OUT_WIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_WIDTH - 1, idx * OUT_WIDTH);
        outStream << tmpValue;
    }
}