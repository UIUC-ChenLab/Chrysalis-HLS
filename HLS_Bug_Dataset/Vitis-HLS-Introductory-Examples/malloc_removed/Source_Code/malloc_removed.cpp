dout_t malloc_removed(din_t din[N], dsel_t width) {

#ifdef NO_SYNTH
  long long *out_accum = malloc(sizeof(long long));
  int *array_local = malloc(64 * sizeof(int));
#else
  long long _out_accum;
  long long *out_accum = &_out_accum;
  int _array_local[64];
  int *array_local = &_array_local[0];
#endif
  int i, j;

LOOP_SHIFT:
  for (i = 0; i < N - 1; i++) {
    if (i < width)
      *(array_local + i) = din[i];
    else
      *(array_local + i) = din[i] >> 2;
  }

  *out_accum = 0;
LOOP_ACCUM:
  for (j = 0; j < N - 1; j++) {
    *out_accum += *(array_local + j);
  }

  return *out_accum;
}