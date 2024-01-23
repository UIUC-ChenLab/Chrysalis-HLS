void streamDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                SIZE_DT inputSize,
                SIZE_DT numItr,
                SIZE_DT blckSize,
                hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm) {
    /**
     * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
     *        N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param in            input hls stream
     * @param inStream_dm   output kernel stream
     * @param inputSize     size of data in to be transferred
     * @param numItr        number of iterations for single file
     * @param blckSize      Block Size
     */
    // read data from input hls to input stream for decompression kernel

    constexpr int c_size = STREAMDWIDTH / 8;
    ap_axiu<STREAMDWIDTH, 0, 0, 0> inData;
    // Loop over multiple iterations of the same file
    for (auto z = 0; z < numItr; z++) {
        // Break file into smaller chunks of multiple TLAST
        for (auto j = 0; j < inputSize; j += blckSize) {
            SIZE_DT bSize = blckSize;
            SIZE_DT inputAlignedSize = bSize / c_size;
            uint8_t inputModSize = 0;
            if (j + blckSize > inputSize) {
                bSize = inputSize - j;
                inputAlignedSize = bSize / c_size;
                inputModSize = bSize % c_size;
            }
        alignedTransfer:
            // Block Size Aligned Data Transfer
            for (auto i = 0; i < inputAlignedSize; i++) {
#pragma HLS PIPELINE
                ap_uint<STREAMDWIDTH> v = in.read();
                inData.data = v;
                inData.keep = -1;
                inData.last = false;
                if (inputModSize == 0 && i == inputAlignedSize - 1) {
                    inData.last = true;
                }
                inStream_dm << inData;
            }
            // Unaligned Data Transfer
            if (inputModSize) {
                ap_uint<STREAMDWIDTH> v = in.read();
                inData.data = v;
                inData.last = true;
                uint32_t num = 0;
                for (auto b = 0; b < inputModSize; b++) {
                    num |= 1UL << b;
                }
                inData.keep = num;
                inStream_dm << inData;
            }
        }
    }
}