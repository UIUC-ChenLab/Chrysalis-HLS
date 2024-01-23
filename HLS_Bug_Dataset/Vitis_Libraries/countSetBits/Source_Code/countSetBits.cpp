uint8_t countSetBits(ap_uint<DWIDTH> val) {
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < DWIDTH; ++i) {
#pragma HLS UNROLL
        cnt += val.range(i, i);
    }
    return cnt;
}