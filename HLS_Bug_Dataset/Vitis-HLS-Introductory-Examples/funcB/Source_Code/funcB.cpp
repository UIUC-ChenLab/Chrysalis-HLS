
#define N 100

void funcB(data_t* in, data_t* out) {
#pragma HLS inline off
Loop0:
    for (int i = 0; i < N; i++) {
#pragma HLS pipeline
        out[i] = in[i] + 25;
    }
}