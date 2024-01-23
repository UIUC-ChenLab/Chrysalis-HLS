#define N 10

void dut(A in[N], A out[N]) {
#pragma HLS interface axis port = in
#pragma HLS interface axis port = out
#pragma HLS disaggregate variable = in
#pragma HLS disaggregate variable = out
    int sum = 0;
    for (unsigned i = 0; i < N; i++) {
        out[i].c = in[i].c;
        out[i].i = in[i].i;
    }
}