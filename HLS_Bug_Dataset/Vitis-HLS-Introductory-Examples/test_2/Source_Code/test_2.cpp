int test(int i) {
    static const ap_int<10> A[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
#pragma HLS BIND_STORAGE variable = A type = ROM_1P impl = BRAM
    static const ap_int<10> B[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
#pragma HLS BIND_STORAGE variable = B type = ROM_1P impl = LUTRAM
    return A[i] + B[i];
}