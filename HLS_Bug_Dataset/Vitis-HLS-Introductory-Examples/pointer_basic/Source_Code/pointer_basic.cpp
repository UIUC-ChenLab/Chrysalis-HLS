void pointer_basic(dio_t *d) {
  static dio_t acc = 0;

  acc += *d;
  *d = acc;
}