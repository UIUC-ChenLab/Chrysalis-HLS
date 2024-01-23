void pointer_arith(dio_t *d) {
  static int acc = 0;
  int i;

  for (i = 0; i < 4; i++) {
    acc += *(d + i + 1);
    *(d + i) = acc;
  }
}