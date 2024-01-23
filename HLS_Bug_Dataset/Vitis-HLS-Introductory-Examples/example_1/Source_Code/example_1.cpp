#define N 10

void example(int a[N], int b[N]) {
#pragma HLS INTERFACE m_axi port = a depth = N bundle =                        \
    gmem max_widen_bitwidth = MAXWBW
#pragma HLS INTERFACE m_axi port = b depth = N bundle = gmem
    int buff[N];
    for (size_t i = 0; i < N; ++i) {
#pragma HLS PIPELINE II = 1
        buff[i] = a[i];
        buff[i] = buff[i] + 100;
        b[i] = buff[i];
    }
}