void bypass(int tmp1[128], int tmp2[128], int tmp3[128]) {

    for (int i = 0; i < 128; i++) {
        tmp3[i] = tmp1[i] + tmp2[i];
    }
}