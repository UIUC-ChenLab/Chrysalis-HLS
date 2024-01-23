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