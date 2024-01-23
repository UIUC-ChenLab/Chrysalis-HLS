void example(int A[50], int B[50]) {
#pragma HLS INTERFACE axis port = A
#pragma HLS INTERFACE axis port = B

    int i;

    for (i = 0; i < 50; i++) {
        B[i] = A[i] + 5;
    }
}