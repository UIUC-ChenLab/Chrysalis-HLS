#define SZ 8

void free_pipe_mult(data_t A[SZ], hls::stream<data_t>& strm, data_t* out) {
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_fifo port = strm

    data_t B[SZ];

    for (int i = 0; i < SZ; i++)
        B[i] = A[i] + i;

    hls::stream<data_t> strm_out;
    process(strm, strm_out);
    inner(B, strm_out, out);
}

// Content of called function
#define SZ 8

void process(hls::stream<data_t>& strm_in, hls::stream<data_t>& strm_out) {
#pragma HLS inline off

    for (int i = 0; i < SZ; i++) {
        data_t tmp;
        tmp = strm_in.read();
        strm_out.write(tmp);
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

// Content of called function
#define SZ 8

void inner(data_t A[SZ], hls::stream<data_t>& stream_in, data_t* out) {
#pragma HLS pipeline

#pragma HLS INTERFACE ap_fifo port = stream_in
    data_t regA[SZ];
#pragma HLS ARRAY_PARTITION variable = regA complete
    for (int i = 0; i < SZ; i++) {
        data_t tmp;
        tmp = stream_in.read();
        regA[i] = A[i] + tmp;
    }

    *out = accumulate(regA);
}

// Content of called function
#define SZ 8

int accumulate(data_t A[]) {
#pragma HLS inline off

    data_t acc = 0.0;
    for (int i = 0; i < SZ; i++) {
        std::cout << "A: " << A[i] << std::endl;
        acc += A[i];
    }
    return acc;
}