void zlibTreegenScheduler(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77InTree[NUM_BLOCK],
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77OutTree[NUM_TREEGEN],
                          hls::stream<ap_uint<4> >& coreIdxStream,
                          hls::stream<uint8_t>& outIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    ap_uint<NUM_BLOCK> is_pending;
    bool eos_tmp[NUM_BLOCK];
    static uint8_t treeIdx = 0;
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
        eos_tmp[i] = false;
    }
    bool read_flag = true;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> inVal;
    ap_uint<4> core = coreIdxStream.read();
    while (is_pending) {
        for (uint8_t i = core; i < NUM_BLOCK + core; i++) {
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
                    lz77OutTree[treeIdx] << inVal;
                    read_flag = true;
                }
                treeIdx++;
                if (treeIdx == NUM_TREEGEN) treeIdx = 0;
            }
        }
    }
    inVal.strobe = 0;
    for (auto i = 0; i < NUM_TREEGEN; i++) {
        lz77OutTree[i] << inVal;
    }
    outIdxNum << 0xFF;
}