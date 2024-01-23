void separateLitSeqTableStreams(hls::stream<IntVectorStream_dt<36, 1> >& fseTableStream,
                                hls::stream<IntVectorStream_dt<36, 1> >& fseLitTableStream,
                                hls::stream<IntVectorStream_dt<36, 1> >& fseSeqTableStream) {
    constexpr uint8_t c_hfIdx = 3; // index of data for literal bitlens
    uint8_t cIdx = 0;
    IntVectorStream_dt<36, 1> fseTableVal;
sep_lit_seq_fset_outer:
    while (true) {
        fseTableVal = fseTableStream.read();
        if (fseTableVal.strobe == 0) break;
    sep_lit_seq_fseTableStreams:
        while (fseTableVal.strobe > 0) {
#pragma HLS PIPELINE II = 1
            if (cIdx == c_hfIdx) {
                fseLitTableStream << fseTableVal;
            } else {
                fseSeqTableStream << fseTableVal;
            }
            fseTableVal = fseTableStream.read();
        }
        // write strobe 0 value
        if (cIdx == c_hfIdx) {
            fseLitTableStream << fseTableVal;
        } else {
            fseSeqTableStream << fseTableVal;
        }
        ++cIdx;
        if (cIdx == 4) cIdx = 0;
    }
    fseTableVal.strobe = 0;
    fseLitTableStream << fseTableVal;
    fseSeqTableStream << fseTableVal;
}