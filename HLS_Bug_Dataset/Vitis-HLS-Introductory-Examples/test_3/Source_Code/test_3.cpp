int test(int i) {
    static TestStruct<10> ts[2] = {{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
                                    {8, 8, 7, 7, 5, 4, 3, 2, 1, 0}},
                                   {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
                                    {9, 9, 8, 6, 5, 4, 3, 2, 1, 0}}};

#pragma HLS BIND_STORAGE variable = ts type = RAM_2P impl = BRAM
    // #pragma HLS BIND_STORAGE variable=ts type=RAM_2P impl=LUTRAM
    // URAMs are not supported for global/static arrays
    // #pragma HLS BIND_STORAGE variable=ts type=RAM_2P impl=URAM

    int ind = i % 2;
    ts[ind].A[i] += ts[ind].B[i] + ts[ind].C[i];
    ts[ind].B[i] += 5;
    ts[ind].C[i] += 10;

    int result = (ts[ind].A[i] + ts[ind].B[i] + ts[ind].C[i]);
    return result;
}