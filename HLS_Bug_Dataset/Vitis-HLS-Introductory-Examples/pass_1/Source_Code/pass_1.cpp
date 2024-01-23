void pass(int a[128], int b[128], int tmp1[128], int tmp2[128]) {
    for (int i = 0; i < 128; i++) {
        tmp1[i] = a[i];
        tmp2[i] = b[i];
    }
}