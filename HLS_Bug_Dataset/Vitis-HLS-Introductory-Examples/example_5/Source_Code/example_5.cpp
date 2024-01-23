void example(hls::stream<trans_pkt>& inStreamTop, ap_uint<64> outTop[1024]) {
#pragma HLS INTERFACE axis register_mode = both register port = inStreamTop
#pragma HLS INTERFACE m_axi max_write_burst_length = 256 latency = 10 depth =  \
    1024 bundle = gmem0 port = outTop
#pragma HLS INTERFACE s_axilite port = outTop bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATAFLOW

    hls::stream<data, DATA_DEPTH> buf;
    hls::stream<int, COUNT_DEPTH> count;

    getinstream(inStreamTop, buf, count);
    streamtoparallelwithburst(buf, count, outTop);
}

// Content of called function
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

// Content of called function
void getinstream(hls::stream<trans_pkt>& in_stream,
                 hls::stream<data>& out_stream, hls::stream<int>& out_counts) {
    int count = 0;
    trans_pkt in_val;
    do {
#pragma HLS PIPELINE
        in_val = in_stream.read();
        data out_val = {in_val.data, in_val.last};
        out_stream.write(out_val);
        count++;
        if (count >= MAX_BURST_LENGTH || in_val.last) {
            out_counts.write(count);
            count = 0;
        }
    } while (!in_val.last);
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