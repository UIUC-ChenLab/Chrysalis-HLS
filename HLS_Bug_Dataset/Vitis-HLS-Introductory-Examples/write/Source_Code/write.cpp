void write(A buf_in[NUM], A* a_out) {
WRITE:
    for (int k = 0; k < NUM; k++) {
        a_out[k] = buf_in[k];
    }
}