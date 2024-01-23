dout_t loop_var(din_t A[N], dsel_t width) {

    dout_t out_accum = 0;
    dsel_t x;

LOOP_X:
    for (x = 0; x < width; x++) {
        out_accum += A[x];
    }

    return out_accum;
}