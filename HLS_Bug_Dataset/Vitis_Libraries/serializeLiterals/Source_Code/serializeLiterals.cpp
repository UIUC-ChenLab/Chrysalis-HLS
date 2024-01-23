void serializeLiterals(hls::stream<ap_uint<68> > litUpsizedStream[CORE_COUNT],
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> > litCntStream[CORE_COUNT],
                       hls::stream<ap_uint<68> >& outLitUpsizedStream,
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outLitCntStream) {
    bool allDone = false;
    ap_uint<68> litVal;
    uint8_t cIdx = 0;
srlz_lit_all_data:
    while (!allDone) {
    // get upsized literals from each core
    srlz_lit_all_cores:
        for (cIdx = 0; cIdx < CORE_COUNT; ++cIdx) {
            litVal = litUpsizedStream[cIdx].read();
            if (litVal.range(3, 0) == 0) { // break if first read is zero
                allDone = true;
                break;
            }
            // write first word (used for continuation or termination)
            outLitUpsizedStream << litVal;
            // get literal count
            auto litCnt = litCntStream[cIdx].read();
            outLitCntStream << litCnt;
        srlz_lit_loop:
            for (litVal = litUpsizedStream[cIdx].read(); litVal.range(3, 0) > 0;
                 litVal = litUpsizedStream[cIdx].read()) {
#pragma HLS PIPELINE II = 1
                outLitUpsizedStream << litVal;
            }
            // end of block
            outLitUpsizedStream << 0;
        }
    }
    // write strobe 0 to output
    outLitUpsizedStream << 0;
// dump strobe 0
srlz_lit_eos_dump:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
#pragma HLS PIPELINE II = 1
        if (i != cIdx) litUpsizedStream[i].read();
    }
}