void mm2Stream(const ap_uint<DATAWIDTH>* in,
               hls::stream<ap_uint<DATAWIDTH> >& outstream,
               hls::stream<ap_uint<32> >& checksumStream,
               uint32_t* checksumData,
               uint32_t inputSize,
               hls::stream<uint32_t>& outSizeStream,
               bool checksumType,
               hls::stream<ap_uint<2> >& checksumTypeStream) {
    /**
     * @brief Read data from DATAWIDTH wide axi memory interface and
     *        write to stream.
     *
     * @tparam DATAWIDTH    width of data bus
     *
     * @param in            pointer to input memory
     * @param outstream     output stream
     * @param inputSize     size of the data
     * @param outSizeStream output size stream
     *
     */
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

    checksumTypeStream << checksumType;
    // exit condition for checksum kernel
    checksumTypeStream << 3;

    outSizeStream << inputSize;
    checksumStream << checksumData[0];

    uint32_t allignedwidth = inSize_gmemwidth / BURST_SIZE;
    allignedwidth = ((inSize_gmemwidth - allignedwidth) > 0) ? allignedwidth + 1 : allignedwidth;

    uint32_t i = 0;
    ap_uint<DATAWIDTH> temp;
mm2s:
    for (; i < allignedwidth * BURST_SIZE; i += BURST_SIZE) {
        for (uint16_t j = 0; j < BURST_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            temp = in[i + j];
            if ((i + j) < inSize_gmemwidth) outstream << temp;
        }
    }
}