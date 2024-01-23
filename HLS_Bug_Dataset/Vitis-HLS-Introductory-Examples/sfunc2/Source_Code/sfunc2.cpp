
#define N 10

void sfunc2(din_t vec1[N], const din_t sIter, din_t ovec[N]) {
    for (int i = 0; i < N; ++i)
        ovec[i] = vec1[i] / sIter;
}