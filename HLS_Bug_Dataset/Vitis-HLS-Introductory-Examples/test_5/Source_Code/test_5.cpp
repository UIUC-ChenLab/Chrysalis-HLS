int test(int i) {
    static TestStruct<10> ts = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
                                {9, 8, 7, 6, 5, 4, 3, 2, 1, 0}};

#pragma HLS BIND_STORAGE variable = ts.A type = RAM_2P impl = BRAM
#pragma HLS BIND_STORAGE variable = ts.B type = RAM_2P impl = LUTRAM

// URAM on Versal devices can be initialized
#pragma HLS BIND_STORAGE variable = ts.C type = RAM_2P impl = URAM

    ts.A[i] += ts.B[i] + ts.C[i];
    ts.B[i] += 5;
    ts.C[i] += 10;

    return (ts.A[i] + ts.B[i] + ts.C[i]);
}