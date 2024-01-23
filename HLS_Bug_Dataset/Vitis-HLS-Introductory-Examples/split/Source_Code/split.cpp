void split(int a[128], int tmp1[128], int tmp2[128]) {
    static int j;
    for (int i = 0; i < 128; i++) {
        j++;
        tmp1[i] = a[i];
        tmp2[i] = a[i];
    }
}