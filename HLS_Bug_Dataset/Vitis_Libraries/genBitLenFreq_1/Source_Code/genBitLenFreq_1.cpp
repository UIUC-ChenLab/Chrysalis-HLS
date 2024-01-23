void genBitLenFreq(Codeword* outCodes, Frequency<MAX_FREQ_DWIDTH>* blFreq, uint16_t maxCode) {
    //#pragma HLS inline
    // generate bit-length frequencies using literal and distance bit-lengths
    ap_uint<4> tree_len[c_litCodeCount];

copy_blens:
    for (int i = 0; i <= maxCode; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = (uint8_t)outCodes[i].bitlength;
    }
clear_rem_blens:
    for (int i = maxCode + 2; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = 0;
    }
    tree_len[maxCode + 1] = (uint8_t)0xff;

    int16_t prevlen = -1;
    int16_t curlen = 0;
    int16_t count = 0;
    int16_t max_count = 7;
    int16_t min_count = 4;
    int16_t nextlen = tree_len[0];

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

parse_tdata:
    for (uint32_t n = 0; n <= maxCode; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 3
        curlen = nextlen;
        nextlen = tree_len[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            blFreq[curlen] += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) {
                blFreq[curlen]++;
            }
            blFreq[c_reusePrevBlen]++;
        } else if (count <= 10) {
            blFreq[c_reuseZeroBlen]++;
        } else {
            blFreq[c_reuseZeroBlen7]++;
        }

        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}