void inflateMultiByte(hls::stream<ap_axiu<16, 0, 0, 0> >& inaxistream,
                      hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& outaxistream) {
    const int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<16> > axi2HlsStrm("axi2HlsStrm");
    hls::stream<bool> axi2HlsEos("axi2HlsEos");
    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > inflateOut("inflateOut");
    hls::stream<uint64_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = axi2HlsStrm depth = 32
#pragma HLS STREAM variable = axi2HlsEos depth = 32
#pragma HLS STREAM variable = inflateOut depth = 32

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = axi2HlsEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inflateOut type = fifo impl = SRL

#pragma HLS dataflow
    details::kStreamReadZlibDecomp<16>(inaxistream, axi2HlsStrm, axi2HlsEos);

    details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
        axi2HlsStrm, axi2HlsEos, inflateOut);

    details::kStreamWriteZlibDecomp<c_parallelBit>(outaxistream, inflateOut);
}

// Content of called function
void kStreamReadZlibDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& in,
                           hls::stream<ap_uint<STREAM_WIDTH> >& out,
                           hls::stream<bool>& outEos) {
    /**
     * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
     *                              and write to output axi stream.
     *
     * @param inKStream     input kernel stream
     * @param readStream    internal stream to be read for processing
     * @param input_size    input data size
     *
     */
    bool last = false;
    while (last == false) {
#pragma HLS PIPELINE II = 1
        ap_axiu<STREAM_WIDTH, 0, 0, 0> tmp = in.read();
        out << tmp.data;
        last = tmp.last;
        outEos << 0;
    }
    out << 0;
    outEos << 1; // Terminate condition
}