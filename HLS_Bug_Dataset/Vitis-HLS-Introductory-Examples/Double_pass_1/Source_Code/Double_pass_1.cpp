void Double_pass(int tmp2[128], int tmp1[128], int tmp4[128], int tmp5[128]) {

    int buff[127];
    for (int i = 0; i < 128; i++) {
        tmp4[i] = tmp1[i];
        tmp5[i] = tmp2[i];
    }
}