void store(hls::stream<vecOf16Words>& in, vecOf16Words* out, int size) {
Loop_St:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out[i] = in.read();
    }
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}