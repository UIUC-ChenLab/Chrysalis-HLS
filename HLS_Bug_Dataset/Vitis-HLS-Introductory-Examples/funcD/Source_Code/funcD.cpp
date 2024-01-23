
#define N 100

void funcD(data_t* in1, data_t* in2, data_t* out) {
#pragma HLS inline off
Loop0:
    for (int i = 0; i < N; i++) {
#pragma HLS pipeline
        out[i] = in1[i] + in2[i] * 2;
    }
}