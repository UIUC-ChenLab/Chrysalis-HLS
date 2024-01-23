
#define N 10

void sfunc1(const din_t cIter, din_t a[N], din_t b[N], din_t res[N]) {
    int i;
    din_t res1[N];
#pragma HLS ARRAY_RESHAPE variable = res1 dim = 1 factor = 2 block
#pragma HLS BIND_STORAGE variable = res1 type = ram_s2p impl = uram_ecc
    for (i = 0; i < N; i++) {
        res1[i] = b[i] + a[i];
    }
    sfunc2(res1, cIter, res);
}

// Content of called function

#define N 10

void sfunc2(din_t vec1[N], const din_t sIter, din_t ovec[N]) {
    for (int i = 0; i < N; ++i)
        ovec[i] = vec1[i] / sIter;
}