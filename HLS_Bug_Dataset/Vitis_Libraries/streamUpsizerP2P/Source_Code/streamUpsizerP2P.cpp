void streamUpsizerP2P(hls::stream<ap_uint<PACK_WIDTH> >& inStream,
                      hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                      hls::stream<uint32_t>& inStreamSize,
                      hls::stream<uint32_t>& outStreamSize) {
    const int c_byte_width = 8;
    const int c_upsize_factor = OUT_WIDTH / c_byte_width;
    const int c_in_size = PACK_WIDTH / c_byte_width;

    // Declaring double buffers
    ap_uint<2 * OUT_WIDTH> outBuffer = 0;
    uint32_t byteIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // printf("Size %d \n", size);
        // Rounding off the output size
        uint32_t outSize = (size * c_byte_width + byteIdx) / PACK_WIDTH;

        if (outSize) outStreamSize << outSize;
    streamUpsizer:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            // printf("val/size %d/%d \n", i, size);
            ap_uint<PACK_WIDTH> tmpValue = inStream.read();
            outBuffer.range((byteIdx + c_in_size) * c_byte_width - 1, byteIdx * c_byte_width) = tmpValue;
            byteIdx += c_byte_width;

            if (byteIdx >= c_upsize_factor) {
                outStream << outBuffer.range(OUT_WIDTH - 1, 0);
                outBuffer >>= OUT_WIDTH;
                byteIdx -= c_upsize_factor;
            }
        }
    }

    if (byteIdx) {
        outStreamSize << 1;
        outStream << outBuffer.range(OUT_WIDTH - 1, 0);
    }
    // printf("%s Done \n", __FUNCTION__);
    // end of block
    outStreamSize << 0;
}

// Content of called function
void streamUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                   hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                   SIZE_DT original_size) {
    /**
     * @brief This module reads IN_WIDTH from the input stream and accumulate
     * the consecutive reads until OUT_WIDTH and writes the OUT_WIDTH data to
     * output stream
     *
     * @tparam SIZE_DT stream size class instance
     * @tparam IN_WIDTH input data width
     * @tparam OUT_WIDTH output data width
     *
     * @param inStream input stream
     * @param outStream output stream
     * @param original_size original stream size
     */

    if (original_size == 0) return;

    uint8_t paralle_byte = IN_WIDTH / 8;
    ap_uint<OUT_WIDTH> shift_register;
    uint8_t factor = OUT_WIDTH / IN_WIDTH;
    original_size = (original_size - 1) / paralle_byte + 1;
    uint32_t withAppendedDataSize = (((original_size - 1) / factor) + 1) * factor;

    for (uint32_t i = 0; i < withAppendedDataSize; i++) {
#pragma HLS PIPELINE II = 1
        if (i != 0 && i % factor == 0) {
            outStream << shift_register;
            shift_register = 0;
        }
        if (i < original_size) {
            shift_register.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = inStream.read();
        } else {
            shift_register.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = 0;
        }
        if ((i + 1) % factor != 0) shift_register >>= IN_WIDTH;
    }
    // write last data to stream
    outStream << shift_register;
}