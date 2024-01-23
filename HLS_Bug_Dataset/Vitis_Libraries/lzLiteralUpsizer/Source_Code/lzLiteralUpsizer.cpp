void lzLiteralUpsizer(hls::stream<ap_uint<10> >& inStream, hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream) {
    const uint8_t c_parallelBit = PARALLEL_BYTES * 8;
    const uint8_t c_maxLitLen = 128;
    ap_uint<c_parallelBit> outBuffer;
    ap_uint<4> idx = 0;

    ap_uint<2> status = 0;
    ap_uint<10> val;
    bool done = false;
lzliteralUpsizer:
    while (status != 2) {
#pragma HLS PIPELINE II = 1
        status = 0;
        val = inStream.read();
        status = val.range(1, 0);
        outBuffer.range((idx + 1) * 8 - 1, idx * 8) = val.range(9, 2);
        idx++;

        if ((status & 1) || (idx == 8)) {
            if (status != 3) {
                litStream << outBuffer;
            }
            idx = 0;
        }
    }
    if (idx > 1) {
        litStream << outBuffer;
        idx = 0;
    }
}