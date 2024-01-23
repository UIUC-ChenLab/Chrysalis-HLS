
#define N 10

void sfunc3(din_t inrun[N], din_t oval[N]) {
    int i;
    din_t calc = 0;
    for (i = 0; i < N; ++i) {
        calc = (inrun[i] * inrun[i]) / (i + 1);
        oval[i] = calc;
    }
}