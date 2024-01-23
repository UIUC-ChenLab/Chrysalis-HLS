void pointer_stream_better(volatile dout_t *d_o, volatile din_t *d_i) {
  din_t acc = 0;

  acc += *d_i;
  acc += *d_i;
  *d_o = acc;
  acc += *d_i;
  acc += *d_i;
  *d_o = acc;
}