void zlibTreegenScheduler(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77InTree[NUM_BLOCK],
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& lz77OutTree,
                          hls::stream<uint8_t>& outIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    ap_uint<NUM_BLOCK> is_pending;
    bool eos_tmp[NUM_BLOCK];
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
        is_pending.range(i, i) = 1;
        eos_tmp[i] = false;
    }
    bool read_flag = true;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> inVal;
    while (is_pending) {
        for (uint8_t i = 0; i < NUM_BLOCK; i++) {
            ap_uint<4> j = i % NUM_BLOCK;
            if (eos_tmp[j] == false) {
                inVal = lz77InTree[j].read();
                read_flag = false;
            }
            bool eos = eos_tmp[j] ? eos_tmp[j] : (inVal.strobe == 0);
            is_pending.range(j, j) = eos ? 0 : 1;
            eos_tmp[j] = eos;
            if (!eos) {
                outIdxNum << j;
                for (uint16_t k = 0; k < c_litDistCodeCnt; k++) {
                    if (read_flag) inVal = lz77InTree[j].read();
                    lz77OutTree << inVal;
                    read_flag = true;
                }
            }
        }
    }
    inVal.strobe = 0;
    lz77OutTree << inVal;
    outIdxNum << 0xFF;
}