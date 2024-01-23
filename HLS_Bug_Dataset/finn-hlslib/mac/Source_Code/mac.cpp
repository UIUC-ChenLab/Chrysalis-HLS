T mac(T const &a, TC const &c, TD const &d, R const &r, unsigned mmv) {
#pragma HLS inline
  T  res = a;
  for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
    res += mul(c[i], d(i,mmv), r);
  }
  return  res;
}