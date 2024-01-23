#define N 10

void example(hls::burst_maxi<din_t> A, hls::burst_maxi<din_t> B,
             hls::burst_maxi<dout_t> RES, din_t factor) {
#pragma HLS INTERFACE m_axi depth = 800 port = A bundle = bundle1
#pragma HLS INTERFACE m_axi depth = 800 port = B bundle = bundle2
#pragma HLS INTERFACE m_axi depth = 800 port = RES bundle = bundle3

    dout_t x_aux[N];
    dout_t y_aux[N];
    read_a(A, x_aux, factor);
    read_b(B, y_aux, factor);

    process(x_aux, y_aux, factor);

    write(RES, x_aux, y_aux);
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
#define N 10

void read_b(hls::burst_maxi<din_t> IN, dout_t y_aux[N], din_t factor) {
    IN.read_request(
        0,
        N / 4); // request to read N/4 elements from the first element
    IN.read_request(
        N - N / 4,
        N / 4); // request to read N/4 elements from the last quarter element
    dout_t Y_accum = N / 4;
    din_t temp;
    for (int i = 0; i < factor / 2; i++) {
        if (i < N / 4) {
            temp = IN.read();
            Y_accum += temp;
            y_aux[i] = Y_accum;
        } else {
            if (i == N / 4) {
                Y_accum = i;
            }
            temp = IN.read();
            Y_accum += temp;
            y_aux[N - N / 2 + i] = Y_accum + N;
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

// Content of called function
#define N 10

void process(dout_t x_aux[N], dout_t y_aux[N], din_t factor) {

    for (int i = 0; i < N / 4; i++) {
        x_aux[i] = factor + x_aux[i];
        y_aux[i] = factor + y_aux[i];
    }

    for (int i = N - N / 4; i < N; i++) {
        x_aux[i] = x_aux[i] - factor;
        y_aux[i] = y_aux[i] - factor;
    }
}

// Content of called function
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