void streamtoparallelwithburst(hls::stream<data>& in_stream,
                               hls::stream<int>& in_counts,
                               ap_uint<64>* out_memory) {
    data in_val;
    do {
        int count = in_counts.read();
        for (int i = 0; i < count; ++i) {
#pragma HLS PIPELINE
            in_val = in_stream.read();
            out_memory[i] = in_val.data_filed;
        }
        out_memory += count;
    } while (!in_val.last);
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}