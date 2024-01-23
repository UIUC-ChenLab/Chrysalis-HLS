void discardBitStream(bitBufferType& bitbuffer, ap_uint<6>& bits_cntr, ap_uint<6> n_bits) {
    bitbuffer >>= n_bits;
    bits_cntr -= n_bits;
}