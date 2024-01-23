void diamond(vecOf16Words* vecIn, vecOf16Words* vecOut, int size) {
// The depth setting is required for pointer to array in the interface.
#pragma HLS INTERFACE m_axi port = vecIn depth = 32
#pragma HLS INTERFACE m_axi port = vecOut depth = 32

    hls::stream<vecOf16Words> c0, c1, c2, c3, c4, c5;
    assert(size % 16 == 0);

#pragma HLS dataflow
    load(vecIn, c0, size);
    compute_A(c0, c1, c2, size);
    compute_B(c1, c3, size);
    compute_C(c2, c4, size);
    compute_D(c3, c4, c5, size);
    store(c5, vecOut, size);
}

// Content of called function
void compute_D(hls::stream<vecOf16Words>& in1, hls::stream<vecOf16Words>& in2,
               hls::stream<vecOf16Words>& out, int size) {
Loop_D:
    for (data_t i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out.write(in1.read() + in2.read());
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
void compute_B(hls::stream<vecOf16Words>& in, hls::stream<vecOf16Words>& out,
               int size) {
Loop_B:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out.write(in.read() + 25);
    }
}

// Content of called function
void compute_A(hls::stream<vecOf16Words>& in, hls::stream<vecOf16Words>& out1,
               hls::stream<vecOf16Words>& out2, int size) {
Loop_A:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        vecOf16Words t = in.read();
        out1.write(t * 3);
        out2.write(t * 3);
    }
}

// Content of called function
void compute_C(hls::stream<vecOf16Words>& in, hls::stream<vecOf16Words>& out,
               int size) {
Loop_C:
    for (data_t i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out.write(in.read() * 2);
    }
}

// Content of called function
void store(hls::stream<vecOf16Words>& in, vecOf16Words* out, int size) {
Loop_St:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out[i] = in.read();
    }
}

// Content of called function
void load(vecOf16Words* in, hls::stream<vecOf16Words>& out, int size) {
Loop_Ld:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out.write(in[i]);
    }
}