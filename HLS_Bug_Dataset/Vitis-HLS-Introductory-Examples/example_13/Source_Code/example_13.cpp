void example(stream<int>& in, stream<int>& out) {
#pragma HLS INTERFACE mode = ap_ctrl_none port = return
#pragma HLS DATAFLOW

    stream<int, 16> inter[2];
    stream<int, 16> mux_in[2];

    demux(in, inter);
    proc<0>(inter[0], mux_in[0]);
    proc<1>(inter[1], mux_in[1]);
    mux(mux_in, out);
}