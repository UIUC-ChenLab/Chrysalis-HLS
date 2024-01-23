int test(int i) {
#pragma HLS BIND_STORAGE variable = A type = RAM_2P impl = BRAM
#pragma HLS BIND_STORAGE variable = B type = RAM_2P impl = LUTRAM
    // URAM is not a supported implementation type for global arrays
    // #pragma HLS BIND_STORAGE variable=C type=RAM_2P impl=URAM
    A[i] += B[i] + C[i];
    B[i] += 5;
    C[i] += 10;

    int result = (A[i] + B[i] + C[i]).to_int();
    return result;
}