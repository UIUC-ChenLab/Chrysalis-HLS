#define N 10

int dut(A arr[N]) {
#pragma HLS interface ap_fifo port = arr
#pragma HLS aggregate variable = arr
    int sum = 0;
    for (unsigned i = 0; i < N; i++) {
        auto tmp = arr[i];
        sum += tmp.foo[0] + tmp.foo[1] + tmp.foo[2] + tmp.bar;
    }
    return sum;
}