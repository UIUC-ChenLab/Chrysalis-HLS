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

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}