void transfer_kernel(hls::burst_maxi<int> in, hls::burst_maxi<int> out,
                     const int size) {
#pragma HLS INTERFACE m_axi port = in depth = 512 latency = 32 offset =        \
    slave bundle = bundle1
#pragma HLS INTERFACE m_axi port = out depth = 5120 offset = slave latency =   \
    32 bundle = bundle2

    int buf[8192];
    in.read_request(0, size);
    for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
        buf[i] = in.read();
    }

    out.write_request(0, size * NUM);

    for (int i = 0; i < NUM; i++) {
        for (int j = 0; j < size; j++) {
#pragma HLS PIPELINE II = 1
            int a = buf[j];
            out.write(a);
        }
    }
    out.write_response();
}

// Content of called function
void write(A buf_in[NUM], A* a_out) {
WRITE:
    for (int k = 0; k < NUM; k++) {
        a_out[k] = buf_in[k];
    }
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}