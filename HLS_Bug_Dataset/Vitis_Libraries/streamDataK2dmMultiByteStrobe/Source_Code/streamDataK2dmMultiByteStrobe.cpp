void streamDataK2dmMultiByteStrobe(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& out,
                                   hls::stream<bool>& outEoS,
                                   hls::stream<uint32_t>& decompressSize,
                                   hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& dmInStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out               output hls stream
     * @param outEoS            eos for output hls stream
     * @param decompressSize    size of data in streams
     * @param dmInStream        input kernel axi stream to be read
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> inValue;
    constexpr uint8_t c_keepDWidth = PARALLEL_BYTES;
    inValue = dmInStream.read();
    uint32_t outSize = countSetBits<c_keepDWidth>((int)(inValue.keep)); // 0

    while (inValue.last == false) {
#pragma HLS PIPELINE II = 1
        outEoS << 0;
        out << inValue.data;

        // read nextValue
        inValue = dmInStream.read();
        outSize += countSetBits<c_keepDWidth>((int)(inValue.keep)); // PARALLEL_BYTES;
    }
    outEoS << 1;
    out << inValue.data;

    decompressSize << outSize;
}