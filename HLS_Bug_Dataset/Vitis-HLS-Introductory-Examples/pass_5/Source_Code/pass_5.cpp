void pass(int tmp3[128], int tmp2[128], int tmp1[128], int b[128]) {
    for (int i = 0; i < 128; i++) {
        b[i] = tmp1[i];
        tmp2[i] = tmp3[i];
    }
}