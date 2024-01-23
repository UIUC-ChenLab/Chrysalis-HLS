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