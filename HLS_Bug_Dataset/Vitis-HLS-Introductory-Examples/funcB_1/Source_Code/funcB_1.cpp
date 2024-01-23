
#define N 100

void funcB(data_t* in, data_t* out) {
Loop0:
    for (int i = 0; i < N; i++) {
#pragma HLS pipeline rewind
#pragma HLS unroll factor = 2
        out[i] = in[i] + 25;
    }
}