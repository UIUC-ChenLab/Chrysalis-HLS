void shift_func(dint_t* in1, dint_t* in2, dout_t* outA, dout_t* outB) {
    *outA = *in1 >> 1;
    *outB = *in2 >> 2;
}