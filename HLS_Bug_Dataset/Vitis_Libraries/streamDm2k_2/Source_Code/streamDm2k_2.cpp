void streamDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                hls::stream<SIZE_DT>& inSize,
                hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& outStream) {
    /**
     * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
     *        N is passed as template parameter.
     *
     * @tparam STREAMDWIDTH stream data width
     *
     * @param in            input hls stream
     * @param outStream     output kernel stream
     * @param inSize        size of data in to be transferred
     *
     */
    // read data from input hls to input stream for decompression kernel

    constexpr int c_size = STREAMDWIDTH / 8;
    SIZE_DT inputSize = inSize.read();
    SIZE_DT inputAlignedSize = inputSize / c_size;
    uint8_t inputModSize = inputSize % c_size;
alignedTransfer:
    for (auto i = 0; i < inputAlignedSize; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<STREAMDWIDTH> v = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> inData;
        inData.data = v;
        inData.keep = -1;
        inData.last = false;
        if (inputModSize == 0 && i == inputAlignedSize - 1) {
            inData.last = true;
        }
        outStream << inData;
    }

    if (inputModSize) {
        ap_uint<STREAMDWIDTH> v = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> inData;
        inData.data = v;
        inData.last = true;
        uint32_t num = 0;
    keepCalc:
        for (auto b = 0; b < inputModSize; b++) {
            num |= 1UL << b;
        }
        inData.keep = num;
        outStream << inData;
    }
}