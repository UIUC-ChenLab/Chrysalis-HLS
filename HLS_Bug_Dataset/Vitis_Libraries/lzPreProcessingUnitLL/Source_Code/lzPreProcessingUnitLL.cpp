void lzPreProcessingUnitLL(hls::stream<SIZE_DT>& inLitLen,
                           hls::stream<SIZE_DT>& inMatchLen,
                           hls::stream<ap_uint<OWIDTH> >& inOffset,
                           hls::stream<ap_uint<11 + OWIDTH> >& outInfo) {
    SIZE_DT litlen = inLitLen.read();
    SIZE_DT matchlen = inMatchLen.read();
    ap_uint<OWIDTH> offset = inOffset.read();
    ap_uint<OWIDTH> litCount = litlen;

    ap_uint<4> l_litlen = 0;
    ap_uint<4> l_matchlen = 0;
    ap_uint<3> l_stateinfo = 0;
    ap_uint<OWIDTH> l_matchloc = litCount - offset;
    ap_uint<11 + OWIDTH> outVal = 0; // 0-15 Match Loc, 16-19 Match Len, 20-23 Lit length, 24-26 State Info
    bool done = false;
    bool read = false;
    bool fdone = false;

    if (litlen == 0) {
        outVal.range(OWIDTH - 1, 0) = 0;
        outVal.range(OWIDTH + 3, OWIDTH) = matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = 6;
        outInfo << outVal;
        done = true;
        fdone = false;
    }
    while (!done) {
#pragma HLS PIPELINE II = 1
        if (litlen) {
            SIZE_DT val = (litlen > PARALLEL_BYTES) ? (SIZE_DT)PARALLEL_BYTES : litlen;
            litlen -= val;
            l_litlen = val;
            l_matchlen = 0;
            l_stateinfo = 0;
            l_matchloc = 0;
            read = (matchlen || litlen) ? false : true;
        } else {
            l_matchlen = (offset > PARALLEL_BYTES)
                             ? ((matchlen > PARALLEL_BYTES) ? (SIZE_DT)PARALLEL_BYTES : (SIZE_DT)matchlen)
                             : (matchlen > offset) ? (SIZE_DT)offset : (SIZE_DT)matchlen;
            if (offset < 6 * PARALLEL_BYTES) {
                l_stateinfo.range(0, 0) = 1;
                l_stateinfo.range(2, 1) = 1;
            } else {
                l_stateinfo.range(0, 0) = 0;
                l_stateinfo.range(2, 1) = 1;
            }
            l_matchloc = litCount - offset;
            if (offset < PARALLEL_BYTES) {
                offset = offset << 1;
            }
            l_litlen = 0;
            litCount += l_matchlen;
            matchlen -= l_matchlen;
            litlen = 0;
            read = matchlen ? false : true;
        }
        outVal.range(OWIDTH - 1, 0) = l_matchloc;
        outVal.range(OWIDTH + 3, OWIDTH) = l_matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = l_litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = l_stateinfo;
        outInfo << outVal;

        if (read) {
            litlen = inLitLen.read();
            matchlen = inMatchLen.read();
            offset = inOffset.read();
            litCount += litlen;
            if (litlen == 0 && matchlen == 0) {
                done = true;
                fdone = true;
            }
        }
    }
    if (fdone) {
        outVal.range(OWIDTH - 1, 0) = l_matchloc;
        outVal.range(OWIDTH + 3, OWIDTH) = matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = 6;
        outInfo << outVal;
    }
}