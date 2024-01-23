
#define N 100

void funcC(hls::stream_of_blocks<block_data_t>& in,
           hls::stream_of_blocks<block_data_t>& out) {
#pragma HLS INLINE off

Loop0:
    for (data_t i = 0; i < N / NUM_BLOCKS; i++) {
#pragma HLS pipeline II = 5

        // Obtain a read lock on the input channel
        // and a write lock for the output channel
        hls::read_lock<block_data_t> inL(in);
        hls::write_lock<block_data_t> outL(out);
        for (unsigned int j = 0; j < NUM_BLOCKS; ++j)
            outL[j] = inL[j] * 2;
    }
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