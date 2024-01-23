void discardBitStreamLL(bitBufferTypeLL& bitbuffer, ap_uint<6>& bits_cntr, ap_uint<6> n_bits) {
#pragma HLS INLINE off
    bitbuffer >>= n_bits;
    bits_cntr -= n_bits;
}