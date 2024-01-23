int test(int i) {
    static ap_int<10> A[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
#pragma HLS BIND_STORAGE variable = A type = RAM_2P impl = BRAM
    static ap_int<10> B[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
#pragma HLS BIND_STORAGE variable = B type = RAM_2P impl = LUTRAM
    static ap_int<10> C[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
#pragma HLS BIND_STORAGE variable = C type = RAM_2P impl = URAM
    A[i] += B[i] + C[i];
    B[i] += 5;
    C[i] += 10;

    int result = (A[i] + B[i] + C[i]).to_int();
    return result;
}