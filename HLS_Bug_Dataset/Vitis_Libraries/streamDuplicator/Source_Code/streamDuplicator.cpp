void streamDuplicator(hls::stream<ap_uint<DWIDTH> >& inHlsStream, hls::stream<ap_uint<DWIDTH> > outStream[SLAVES]) {
    ap_uint<DWIDTH> tempVal = inHlsStream.read();
    for (uint8_t i = 0; i < SLAVES; i++) {
#pragma HLS UNROLL
        outStream[i] << tempVal;
    }
}