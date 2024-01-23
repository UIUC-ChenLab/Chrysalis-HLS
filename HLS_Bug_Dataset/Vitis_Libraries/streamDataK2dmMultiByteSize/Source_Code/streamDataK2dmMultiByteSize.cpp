void streamDataK2dmMultiByteSize(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& out,
                                 hls::stream<bool>& outEoS,
                                 hls::stream<SIZE_DT>& decompressSize,
                                 hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& dmInStream,
                                 hls::stream<ap_axiu<AXIWIDTH, 0, 0, 0> >& dmInSizeStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out           output hls stream
     * @param dmOutStream   input kernel axi stream to be read
     * @param dataSize      size of data in streams
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> inValue;
    inValue = dmInStream.read();

    while (inValue.last == false) {
#pragma HLS PIPELINE II = 1
        outEoS << 0;
        out << inValue.data;

        // read nextValue
        inValue = dmInStream.read();
    }

    outEoS << 1;
    out << inValue.data;

    ap_axiu<AXIWIDTH, 0, 0, 0> uncompressSize = dmInSizeStream.read();
    decompressSize << uncompressSize.data;
}