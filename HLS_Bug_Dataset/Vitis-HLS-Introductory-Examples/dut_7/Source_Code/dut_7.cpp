int dut(hls::stream<int> s_in[M], hls::stream<int> s_out[M]) {
#pragma HLS INTERFACE axis port = s_in
#pragma HLS INTERFACE axis port = s_out

    int sum = 0;
    for (unsigned j = 0; j < M; j++) {
        for (unsigned i = 0; i < 10; i++) {
            int val = s_in[j].read();
            s_out[j].write(val + 2);
            sum += val;
        }
    }
    return sum;
}

// Content of called function
#define N 10

void write(hls::burst_maxi<dout_t> RES, dout_t x_aux[N], dout_t y_aux[N]) {
    RES.write_request(
        0, N / 4); // request to write N/4 elements from the first element
    RES.write_request(
        N - N / 4,
        N / 4); // request to write N/4 elements from the last quarter element

    for (int i = 0; i < N / 2; i++) {
        if (i < N / 4)
            RES.write(x_aux[i] - y_aux[i]);
        else
            RES.write(x_aux[N - N / 2 + i] - y_aux[N - N / 2 + i] / N);
    }
    RES.write_response(); // wait for the write operation to complete
    RES.write_response(); // wait for the write operation to complete
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}