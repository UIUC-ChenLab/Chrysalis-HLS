void upsizer_sizestream(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                        hls::stream<SIZE_DT>& inStreamSize,
                        hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                        hls::stream<SIZE_DT>& outStreamSize) {
    // Constants
    const int c_byte_width = 8; // 8bit is each BYTE
    const int c_upsize_factor = OUT_WIDTH / c_byte_width;
    const int c_in_size = IN_WIDTH / c_byte_width;

    ap_uint<2 * OUT_WIDTH> outBuffer = 0; // Declaring double buffers
    uint32_t byteIdx = 0;
    // printme("%s: factor=%d\n",__FUNCTION__,c_upsize_factor);
    for (SIZE_DT size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // rounding off the output size
        uint16_t outSize = ((size + byteIdx) / c_upsize_factor) * c_upsize_factor;
        if (outSize) {
            outStreamSize << outSize;
        }
    ////printme("%s: reading next data=%d outSize=%d c_in_size=%d\n ",__FUNCTION__, size,outSize,c_in_size);
    stream_upsizer:
        for (int i = 0; i < size; i += c_in_size) {
#pragma HLS PIPELINE II = 1
            int chunk_size = c_in_size;
            if (chunk_size + i > size) chunk_size = size - i;
            ap_uint<IN_WIDTH> tmpValue = inStream.read();
            outBuffer.range((byteIdx + c_in_size) * c_byte_width - 1, byteIdx * c_byte_width) = tmpValue;
            byteIdx += chunk_size;
            ////printme("%s: value=%c, chunk_size = %d and byteIdx=%d\n",__FUNCTION__,(char)tmpValue,
            /// chunk_size,byteIdx);
            if (byteIdx >= c_upsize_factor) {
                outStream << outBuffer.range(OUT_WIDTH - 1, 0);
                outBuffer >>= OUT_WIDTH;
                byteIdx -= c_upsize_factor;
            }
        }
    }
    if (byteIdx) {
        outStreamSize << byteIdx;
        ////printme("sent outSize %d \n", byteIdx);
        outStream << outBuffer.range(OUT_WIDTH - 1, 0);
    }
    // end of block
    outStreamSize << 0;
    // printme("%s:Ended \n",__FUNCTION__);
}