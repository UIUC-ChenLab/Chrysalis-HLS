void simpleDownSizer(hls::stream<ap_uint<PARALLEL_BYTES + DATAWIDTH> >& inStream,
                     hls::stream<ap_uint<OUTWIDTH> >& outStream,
                     hls::stream<bool>& outEos) {
    const int c_factor = DATAWIDTH / OUTWIDTH;
    const int c_inWidth = DATAWIDTH + PARALLEL_BYTES;

    ap_uint<OUTWIDTH> outData;
    ap_uint<c_inWidth> inData = inStream.read();
    bool eosFlag = inData.range(PARALLEL_BYTES - 1, 0);
    ap_uint<DATAWIDTH> data = inData.range(c_inWidth - 1, PARALLEL_BYTES);
    int idx = 0;

    while (!eosFlag) {
        outData = data;
        data >>= OUTWIDTH;
        idx += 1;
        if (idx == c_factor) {
            idx = 0;
            inData = inStream.read();
            eosFlag = inData.range(PARALLEL_BYTES - 1, 0);
            data = inData.range(c_inWidth - 1, PARALLEL_BYTES);
        }
        outStream << outData;
        outEos << 0;
    }
    outStream << 0;
    outEos << 1;
}