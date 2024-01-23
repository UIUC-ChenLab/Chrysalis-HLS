void kStreamWriteMultiByteSize(hls::stream<ap_axiu<DATAWIDTH, 0, 0, 0> >& outKStream,
                               hls::stream<ap_axiu<AXIWIDTH, 0, 0, 0> >& outKSizeStream,
                               hls::stream<ap_uint<DATAWIDTH + 8> >& inDataStream) {
    /**
     * @brief Read N-bit wide data from internal hls streams and write to axi
     *        kernel stream for the given size. N is the template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH    data width of the kernel stream
     *
     * @param outKStream    output kernel stream
     * @param outDataStream output data stream from internal modules
     * @param dataSize      size of data in streams
     *
     */
    // read the original size
    const uint8_t c_parallelByte = DATAWIDTH / 8;
    uint32_t outSize = 0;
    ap_uint<DATAWIDTH + 8> inValue;
    ap_axiu<DATAWIDTH, 0, 0, 0> outValue;
    ap_axiu<AXIWIDTH, 0, 0, 0> decompressSize;
    ap_uint<DATAWIDTH> outData;

    ap_uint<c_parallelByte> strb = 1;

    for (; strb != 0;) {
#pragma HLS PIPELINE II = 1
        inValue = inDataStream.read();
        outData = inValue.range(DATAWIDTH + 7, c_parallelByte);
        strb = inValue.range(c_parallelByte - 1, 0);
        size_t size = (32 - __builtin_clz(strb));
        outValue.data = outData;
        outValue.last = (strb == 0) ? true : false;
        outKStream.write(outValue);
        outSize += (strb != 0) ? size : 0;
    }

    decompressSize.data = outSize;
    decompressSize.last = true;
    outKSizeStream.write(decompressSize);
}