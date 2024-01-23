void s2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream, ap_uint<DATAWIDTH>* out, hls::stream<uint32_t>& inStreamSize) {
    const int c_byte_size = 8;
    const int c_factor = DATAWIDTH / c_byte_size;

    uint32_t outIdx = 0;
    uint32_t size = 1;
    uint32_t sizeIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
    mwr:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            out[outIdx + i] = inStream.read();
        }
        outIdx += size;
    }
}