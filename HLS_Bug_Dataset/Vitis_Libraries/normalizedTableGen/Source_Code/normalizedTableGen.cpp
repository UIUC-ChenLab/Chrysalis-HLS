void normalizedTableGen(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                        hls::stream<IntVectorStream_dt<16, 1> > normTableStream[2]) {
    // generate normalized counter to be used for fse table generation
    /*
     * > Read the frequencies
     * > Get max frequency value, maxCode and sequence count
     * > Calculate optimal table log
     * > Calculate normalized distribution table
     */
    const uint8_t c_maxTableLog[4] = {c_fseMaxTableLogLL, c_fseMaxTableLogOF, c_fseMaxTableLogML, c_fseMaxTableLogHF};
    const uint8_t c_hfIdx = 3;
    IntVectorStream_dt<16, 1> outVal;
    ap_uint<MAX_FREQ_DWIDTH> inFreq[64]; // using max possible size for array
    int16_t normTable[64];
#pragma HLS BIND_STORAGE variable = inFreq type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = normTable type = ram_1p impl = lutram
    uint16_t seqCnt = 0;
norm_tbl_outer_loop:
    while (true) {
        bool noSeq = true;
        uint8_t lastSeq[3] = {0, 0, 0}; // ll, ml, of
#pragma HLS ARRAY_PARTITION variable = lastSeq complete
        uint8_t lsk = 0;
        // first value is sequence count
        auto inVal = inFreqStream.read();
        if (inVal.strobe == 0) break;
        seqCnt = inVal.data[0];
        noSeq = (seqCnt == 0);
    // last sequence
    read_last_seq:
        for (uint8_t i = 0; i < 3; ++i) {
#pragma HLS PIPELINE II = 1
            inVal = inFreqStream.read();
            lastSeq[i] = inVal.data[0];
        }
    calc_norm_outer:
        for (uint8_t k = 0; k < 4; ++k) {
        init_norm_table:
            for (uint8_t i = 0; i < 64; ++i) {
#pragma HLS PIPELINE
                normTable[i] = 0;
            }
            uint16_t maxSymbol;
            ap_uint<MAX_FREQ_DWIDTH> maxFreq = 0;
            uint16_t symCnt = seqCnt;
            volatile uint16_t symCnt_1 = seqCnt;
            // read max literal value, before weight freq data
            if (c_hfIdx == k) {
                inVal = inFreqStream.read();
                symCnt = inVal.data[0];
                symCnt_1 = symCnt;
            }
            // number of frequencies to read
            inVal = inFreqStream.read();
            uint16_t freqCnt = inVal.data[0];
        // read input frequencies
        read_in_freq:
            for (uint16_t i = 0; i < freqCnt; ++i) {
#pragma HLS PIPELINE II = 1
                inVal = inFreqStream.read();
                inFreq[i] = inVal.data[0];
                if (inVal.data[0] > 0) maxSymbol = i;
                if (inVal.data[0] > maxFreq) maxFreq = inVal.data[0];
            }
            uint8_t tableLog = 0;
            if (noSeq == false || c_hfIdx == k) {
                // get optimal table log
                tableLog = getOptimalTableLog(c_maxTableLog[k], symCnt, maxSymbol);
                if (c_hfIdx != k) {
                    if (inFreq[lastSeq[lsk]] > 1) {
                        inFreq[lastSeq[lsk]]--;
                        symCnt_1 -= 1;
                    }
                    ++lsk;
                }
                // generate normalized distribution table
                normalizeFreq<MAX_FREQ_DWIDTH>(inFreq, symCnt_1, maxSymbol, tableLog, normTable);
            }
            // write normalized table to output
            outVal.strobe = 1;
            // write tableLog, max val at the end of table log
            normTable[63] = maxSymbol;
            normTable[62] = tableLog;
        // normTable[61] = (int16_t)(c_hfIdx == k); // To be used later for optimization
        write_norm_table:
            for (uint16_t i = 0; i < 64; ++i) {
#pragma HLS PIPELINE II = 1
                outVal.data[0] = normTable[i];
                normTableStream[0] << outVal;
                normTableStream[1] << outVal;
            }
        }
    }
    outVal.strobe = 0;
    normTableStream[0] << outVal;
    normTableStream[1] << outVal;
}