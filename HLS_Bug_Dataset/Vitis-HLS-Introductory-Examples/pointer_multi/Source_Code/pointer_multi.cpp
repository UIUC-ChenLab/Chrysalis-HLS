dout_t pointer_multi(sel_t sel, din_t pos) {
  static dout_t a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  static dout_t b[8] = {8, 7, 6, 5, 4, 3, 2, 1};

  dout_t *ptr;
  if (sel)
    ptr = a;
  else
    ptr = b;

  return ptr[pos];
}