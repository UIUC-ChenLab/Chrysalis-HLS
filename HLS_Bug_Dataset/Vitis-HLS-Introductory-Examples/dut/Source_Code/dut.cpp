#define N 10

int dut(A* arr) {
#pragma HLS interface m_axi port = arr depth = 10
#pragma HLS interface s_axilite port = arr
#pragma HLS aggregate variable = arr compact = auto
    int sum = 0;
    for (unsigned i = 0; i < N; i++) {
        auto tmp = arr[i];
        sum += tmp.foo + tmp.bar;
    }
    return sum;
}