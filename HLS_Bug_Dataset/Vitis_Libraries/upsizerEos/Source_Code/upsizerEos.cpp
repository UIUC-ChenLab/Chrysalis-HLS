void upsizerEos(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                hls::stream<bool>& inStream_eos,
                hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                hls::stream<bool>& outStream_eos) {
    /**
     * @brief This module reads IN_WIDTH data from input stream based
     * on end of stream and accumulate the consecutive reads until
     * OUT_WIDTH and then writes OUT_WIDTH data to output stream.
     *
     * @tparam IN_WIDTH input data width
     * @tparam OUT_WIDTH output data width
     *
     * @param inStream input stream
     * @param inStream_eos input end of stream flag
     * @param outStream output stream
     * @param outStream_eos output end of stream flag
     */
    // Constants
    const int c_byteWidth = IN_WIDTH;
    const int c_upsizeFactor = OUT_WIDTH / c_byteWidth;
    const int c_inSize = IN_WIDTH / c_byteWidth;

    ap_uint<OUT_WIDTH> outBuffer = 0;
    ap_uint<IN_WIDTH> outBuffer_int[c_upsizeFactor];
#pragma HLS array_partition variable = outBuffer_int dim = 1 complete
    uint32_t byteIdx = 0;
    bool done = false;
    ////printme("%s: reading next data=%d outSize=%d c_inSize=%d\n ",__FUNCTION__, size,outSize,c_inSize);
    outBuffer_int[byteIdx] = inStream.read();
stream_upsizer:
    for (bool eos_flag = inStream_eos.read(); eos_flag == false; eos_flag = inStream_eos.read()) {
#pragma HLS PIPELINE II = 1
        for (int j = 0; j < c_upsizeFactor; j += c_inSize) {
#pragma HLS unroll
            outBuffer.range((j + 1) * c_byteWidth - 1, j * c_byteWidth) = outBuffer_int[j];
        }
        byteIdx += 1;
        ////printme("%s: value=%c, chunk_size = %d and byteIdx=%d\n",__FUNCTION__,(char)tmpValue, chunk_size,byteIdx);
        if (byteIdx >= c_upsizeFactor) {
            outStream << outBuffer;
            outStream_eos << 0;
            byteIdx -= c_upsizeFactor;
        }
        outBuffer_int[byteIdx] = inStream.read();
    }

    if (byteIdx) {
        outStream_eos << 0;
        outStream << outBuffer;
    }
    // end of block

    outStream << 0;
    outStream_eos << 1;
    // printme("%s:Ended \n",__FUNCTION__);
}