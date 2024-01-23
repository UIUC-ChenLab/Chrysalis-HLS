void loadBitStream(bitBufferType& bitbuffer,
                   ap_uint<6>& bits_cntr,
                   hls::stream<ap_uint<16> >& inStream,
                   hls::stream<bool>& inEos,
                   bool& done) {
#pragma HLS INLINE off
    while (bits_cntr < 32 && (done == false)) {
    loadBitStream:
        uint16_t tmp_dt = (uint16_t)inStream.read();
        bitbuffer += (bitBufferType)(tmp_dt) << bits_cntr;
        done = inEos.read();
        bits_cntr += 16;
    }
}