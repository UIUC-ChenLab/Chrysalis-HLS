T reg(T d) {
#pragma HLS PIPELINE II = 1
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INLINE off
    return d;
}