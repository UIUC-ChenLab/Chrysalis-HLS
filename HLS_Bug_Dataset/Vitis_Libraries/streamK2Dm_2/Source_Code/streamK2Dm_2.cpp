void streamK2Dm(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& out,
                hls::stream<bool>& outEoS,
                hls::stream<SIZE_DT>& dataSize,
                hls::stream<ap_axiu<PARALLEL_BYTES * 8, TUSR_DWIDTH, 0, 0> >& dmInStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
     *
     * @param out           output hls stream
     * @param dmOutStream   input kernel axi stream to be read
     * @param dataSize      size of data in streams
     *
     */
    // read data from decompression kernel output to global memory output
    SIZE_DT cntr = 0;
    ap_axiu<PARALLEL_BYTES * 8, TUSR_DWIDTH, 0, 0> inValue;
    auto keep = 0;
    bool last = false;

dmAxi2hls:
    while (!last) {
#pragma HLS PIPELINE II = 1
        inValue = dmInStream.read();
        outEoS << 0;
        out << inValue.data;
        last = inValue.last;
        keep = inValue.keep;
        if (last) break;
        cntr += PARALLEL_BYTES;
    }

    auto incr = 0;
dmLeftSize:
    while (keep) {
        incr += (keep & 0x1);
        keep >>= 1;
    }

    cntr += incr;
    dataSize << cntr;
    if (TUSR_DWIDTH != 0) dataSize << inValue.user;
    outEoS << 1;
    out << 0;
}