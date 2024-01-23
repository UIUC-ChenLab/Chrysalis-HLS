void genBitLenFreq(hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                   hls::stream<bool>& isEOBlocks,
                   hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& freq,
                   hls::stream<ap_uint<c_tgnSymbolBits> >& maxCode) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    ap_uint<MAX_FREQ_DWIDTH> blFreq[19];
    // iterate over blocks
    while (isEOBlocks.read() == false) {
    init_bitlen_freq:
        for (uint8_t i = 0; i < 19; ++i) {
#pragma HLS PIPELINE off
            blFreq[i] = 0;
        }
        ap_uint<MAX_FREQ_DWIDTH> repPrevBlen = 0;
        ap_uint<MAX_FREQ_DWIDTH> repZeroBlen = 0;
        ap_uint<MAX_FREQ_DWIDTH> repZeroBlen7 = 0;

        for (uint8_t itr = 0; itr < 2; ++itr) {
            int16_t prevlen = -1;
            int16_t curlen = 0;
            int16_t count = 0;
            int16_t max_count = 7;
            int16_t min_count = 4;

            int16_t nextlen = outCodes.read().data[0].bitlength;
            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            ap_uint<c_tgnSymbolBits> maximumCodeLength = maxCode.read();
            ap_uint<MAX_FREQ_DWIDTH> inc = 0;
        parse_tdata:
            for (ap_uint<c_tgnSymbolBits> n = 0; n <= maximumCodeLength; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 2
                curlen = nextlen;
                if (n == maximumCodeLength) {
                    nextlen = 0xF;
                } else {
                    nextlen = outCodes.read().data[0].bitlength;
                }
                if (++count < max_count && curlen == nextlen) {
                    continue;
                } else if (count < min_count) {
                    blFreq[curlen] += count;
                } else if (curlen != 0) {
                    if (curlen != prevlen) {
                        blFreq[curlen]++;
                    }
                    ++repPrevBlen;
                } else if (count <= 10) {
                    ++repZeroBlen;
                } else {
                    ++repZeroBlen7;
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
        read_spare_codes:
            for (auto i = maximumCodeLength + 1; i < hctMeta[itr]; ++i) {
#pragma HLS PIPELINE II = 1
                auto tmp = outCodes.read();
            }
        }
        // repeat freqs
        blFreq[c_reusePrevBlen] = repPrevBlen;
        blFreq[c_reuseZeroBlen] = repZeroBlen;
        blFreq[c_reuseZeroBlen7] = repZeroBlen7;
        // write output
        IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> blf;
        blf.strobe = 1;
    output_bitlen_frequencies:
        for (uint8_t i = 0; i < 19; ++i) {
#pragma HLS PIPELINE II = 1
            blf.data[0] = blFreq[i];
            freq << blf;
        }
    }
}