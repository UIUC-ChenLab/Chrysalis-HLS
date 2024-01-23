void pointer_stream_good(volatile dout_t *d_o, volatile din_t *d_i) {
  din_t acc = 0;

  acc += *d_i;
  acc += *(d_i + 1);
  *d_o = acc;
  acc += *(d_i + 2);
  acc += *(d_i + 3);
  *(d_o + 1) = acc;
}