#define N 10

void read_a(hls::burst_maxi<din_t> IN, dout_t x_aux[N], din_t factor) {

    IN.read_request(
        0,
        N / 4); // request to read N/4 elements from the first element
    IN.read_request(
        N - N / 4,
        N / 4); // request to read N/4 elements from the last quarter element
    dout_t X_accum = N / 4;
    din_t temp;
    for (int i = 0; i < factor / 2; i++) {
        if (i < N / 4) {
            temp = IN.read();
            X_accum += temp;
            x_aux[i] = X_accum;
        } else {
            if (i == N / 4) {
                X_accum = i;
            }
            temp = IN.read();
            X_accum += temp;
            x_aux[N - N / 2 + i] = X_accum - N;
        }
    }
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}