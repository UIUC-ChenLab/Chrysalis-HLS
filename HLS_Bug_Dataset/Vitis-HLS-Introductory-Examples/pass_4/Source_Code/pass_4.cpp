void pass(int tmp1[128], int b[128]) {
    int buff[127];
    for (int i = 0; i < 128; i++) {
        b[i] = tmp1[i];
    }
}