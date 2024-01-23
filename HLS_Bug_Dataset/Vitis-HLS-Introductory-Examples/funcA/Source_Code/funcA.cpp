
#define N 100

void funcA(data_t* in, data_t* out1, data_t* out2) {
#pragma HLS inline off
Loop0:
    for (int i = 0; i < N; i++) {
#pragma HLS pipeline
        data_t t = in[i] * 3;
        out1[i] = t;
        out2[i] = t;
    }
}