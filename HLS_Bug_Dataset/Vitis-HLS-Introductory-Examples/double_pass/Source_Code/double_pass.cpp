void double_pass(int b[128], int tmp2[128], int tmp1[128], int tmp4[128]) {

    int buff[127];
    for (int i = 0; i < 128; i++) {
        tmp2[i] = b[i];
        tmp4[i] = tmp1[i];
    }
}