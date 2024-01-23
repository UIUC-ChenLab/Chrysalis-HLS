void lzBooster(hls::stream<IntVectorStream_dt<32, 1> >& inStream, hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    constexpr uint16_t c_fifo_depth = LEFT_BYTES + 2;
    constexpr int c_boosterOffsetWindow = (BLOCKSIZE < BOOSTER_OFFSET_WINDOW) ? BLOCKSIZE : BOOSTER_OFFSET_WINDOW;
    bool last_block = false;
    bool block_end = true;
    IntVectorStream_dt<32, 1> outStreamValue;
    ap_uint<32> outValue = 0;

    while (true) {
        uint8_t local_mem[c_boosterOffsetWindow];
        hls::stream<ap_uint<32> > lclBufStream("lclBufStream");
#pragma HLS STREAM variable = lclBufStream depth = c_fifo_depth
#pragma HLS BIND_STORAGE variable = lclBufStream type = fifo impl = srl
        uint32_t iIdx = 0;

        uint32_t match_loc = 0;
        uint32_t match_len = 0;

        outStreamValue.strobe = 1;

        bool matchFlag = false;
        bool outFlag = false;
        bool boostFlag = false;
        uint16_t skip_len = 0;
        uint8_t nextMatchCh = local_mem[match_loc % c_boosterOffsetWindow];

        // check and exit if end of data
        auto nextVal = inStream.read();
        if (nextVal.strobe == 0) {
            outStreamValue.strobe = 0;
            outStream << outStreamValue;
            break;
        }
    // assuming that, at least bytes more than LEFT_BYTES will be present at the input
    lz_booster_init_buf:
        for (uint16_t i = 0; i < LEFT_BYTES; ++i) {
#pragma HLS PIPELINE II = 1
            auto tmp = nextVal.data[0];
            nextVal = inStream.read();
            lclBufStream << tmp;
        }

    lz_booster:
        for (; nextVal.strobe != 0; ++iIdx) { // iterate through block data
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = local_mem inter false
#endif
            // read value from fifo
            auto inValue = lclBufStream.read();
            // read from input stream
            auto tmp = inStream.read();
            // write new value to fifo
            lclBufStream << nextVal.data[0];
            // update next val
            nextVal = tmp;

            uint8_t tCh = inValue.range(7, 0);
            uint8_t tLen = inValue.range(15, 8);
            uint16_t tOffset = inValue.range(31, 16);

            if (tOffset < c_boosterOffsetWindow) {
                boostFlag = true;
            } else {
                boostFlag = false;
            }
            uint8_t match_ch = nextMatchCh;
            local_mem[iIdx % c_boosterOffsetWindow] = tCh;
            outFlag = false;

            if (skip_len) {
                skip_len--;
            } else if (matchFlag && (match_len < MAX_MATCH_LEN) && (tCh == match_ch)) {
                match_len++;
                match_loc++;
                outValue.range(15, 8) = match_len;
            } else {
                match_len = 1;
                match_loc = iIdx - tOffset;
                if (iIdx) outFlag = true;
                outStreamValue.data[0] = outValue;
                outValue = inValue;
                if (tLen) {
                    if (boostFlag) {
                        matchFlag = true;
                        skip_len = 0;
                    } else {
                        matchFlag = false;
                        skip_len = tLen - 1;
                    }
                } else {
                    matchFlag = false;
                }
            }
            nextMatchCh = local_mem[match_loc % c_boosterOffsetWindow];
            if (outFlag) {
                outStream << outStreamValue;
            }
        }
        outStreamValue.data[0] = outValue;
        outStream << outStreamValue;
    lz_booster_left_bytes:
        for (uint16_t i = 0; i < LEFT_BYTES; ++i) {
#pragma HLS PIPELINE II = 1
            outStreamValue.data[0] = lclBufStream.read();
            outStream << outStreamValue;
        }
        // Block end
        outStreamValue.strobe = 0;
        outStream << outStreamValue;
    }
}