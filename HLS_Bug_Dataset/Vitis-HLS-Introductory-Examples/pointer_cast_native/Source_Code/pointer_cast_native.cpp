data_t pointer_cast_native(data_t index, data_t A[N]) {
  dint_t *ptr;
  data_t i = 0, result = 0;
  ptr = (dint_t *)(&A[index]);

  // Sum from the indexed value as a different type
  for (i = 0; i < 4 * (N / 10); ++i) {
    result += *ptr;
    ptr += 1;
  }
  return result;
}