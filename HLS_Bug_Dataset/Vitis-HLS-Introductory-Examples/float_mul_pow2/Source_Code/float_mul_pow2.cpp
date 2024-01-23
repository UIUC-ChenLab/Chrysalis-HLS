float float_mul_pow2(float x, int8_t n) {
  float_num_t x_num, prod;

  x_num.fp_num = x;
#ifndef AESL_FP_MATH_NO_BOUNDS_TESTS
  if (x_num.bexp == 0xFF || x_num.bexp == 0) // pass through NaN, INF & denorm
    prod.fp_num = x_num.fp_num;
  else if (n >= 0 && x_num.bexp >= 255 - n) { // detect and handle overflow
    prod.sign = x_num.sign;                   //
    prod.bexp = 0xFF;                         // +/-INF
    prod.mant = 0;                            //
  } else if (n < 0 &&
             x_num.bexp <= ABS(n)) { // handle underflow (doesn't gen denorms)
    prod.sign = x_num.sign;          //
    prod.bexp = 0;                   // +/-ZERO
    prod.mant = 0;                   //
  } else
#endif // AESL_FP_MATH_NO_BOUNDS_TESTS not defined
  {
    prod.sign = x_num.sign;
    prod.bexp = x_num.bexp + n;
    prod.mant = x_num.mant;
  }
  return prod.fp_num;
}