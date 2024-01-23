
#define N 100

void funcC(data_t* in, data_t* out) {
#pragma HLS inline off
Loop0:
    for (data_t i = 0; i < N; i++) {
#pragma HLS pipeline
        out[i] = in[i] * 2;
    }
}