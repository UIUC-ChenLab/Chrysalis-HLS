
#define N 10

void ecc_flags(din_t in1[N], din_t in2[N], const din_t Iter, din_t output[N]) {
    din_t auxbuffer1[N];

    sfunc1(Iter, in1, in2, auxbuffer1);
    sfunc3(auxbuffer1, output);

    return;
}

// Content of called function

#define N 10

void sfunc3(din_t inrun[N], din_t oval[N]) {
    int i;
    din_t calc = 0;
    for (i = 0; i < N; ++i) {
        calc = (inrun[i] * inrun[i]) / (i + 1);
        oval[i] = calc;
    }
}

// Content of called function

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