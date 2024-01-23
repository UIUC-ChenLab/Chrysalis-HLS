void _rdxsrtInitHistogram(ap_uint<SYMBOL_BITS> digit_histogram[RADIX]) {
#pragma HLS INLINE
init_histogram:
    for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS UNROLL
        digit_histogram[i] = 0;
    }
}