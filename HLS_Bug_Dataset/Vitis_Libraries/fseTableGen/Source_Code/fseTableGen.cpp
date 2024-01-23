void fseTableGen(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                 hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream,
                 hls::stream<IntVectorStream_dt<36, 1> >& fseLitTableStream,
                 hls::stream<IntVectorStream_dt<36, 1> >& fseSeqTableStream) {
    // internal streams
    hls::stream<IntVectorStream_dt<16, 1> > normTableStream[2];
    hls::stream<IntVectorStream_dt<36, 1> > fseTableStream("fseTableStream");
#pragma HLS STREAM variable = normTableStream depth = 128
#pragma HLS STREAM variable = fseTableStream depth = 16

#pragma HLS DATAFLOW
    // generate normalized counter table
    normalizedTableGen<MAX_FREQ_DWIDTH>(inFreqStream, normTableStream);
    // generate FSE header
    fseHeaderGen(normTableStream[0], fseHeaderStream);
    // generate FSE encoding tables
    fseEncodingTableGen(normTableStream[1], fseTableStream);
    // separate lit-seq fse table streams
    separateLitSeqTableStreams(fseTableStream, fseLitTableStream, fseSeqTableStream);
}

// Content of called function
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

// Content of called function
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

// Content of called function
void fseHeaderGen(hls::stream<IntVectorStream_dt<16, 1> >& normTableStream,
                  hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream) {
    // generate normalized counter to be used for fse table generation
    int16_t normTable[64];
#pragma HLS BIND_STORAGE variable = normTable type = ram_1p impl = lutram
    IntVectorStream_dt<8, 2> outVal;

fse_header_gen_outer:
    while (true) {
        auto inVal = normTableStream.read();
        if (inVal.strobe == 0) break;
        normTable[0] = inVal.data[0];
    read_norm_table:
        for (uint8_t i = 1; i < 64; ++i) {
#pragma HLS PIPELINE II = 1
            inVal = normTableStream.read();
            normTable[i] = inVal.data[0];
        }
        uint16_t maxCharSize = normTable[63] + 1;
        uint8_t tableLog = normTable[62];
        outVal.strobe = 2;

        if (tableLog > 0) {
            uint16_t tableSize = 1 << tableLog;
            ap_uint<32> bitStream = 0;
            int8_t bitCount = 0;
            uint16_t symbol = 0;
            uint8_t outCount = 0;

            /* Table Size */
            bitStream = (tableLog - c_fseMinTableLog);
            bitCount = 4;

            /* Init */
            int remaining = tableSize + 1; /* +1 for extra accuracy */
            uint16_t threshold = tableSize;
            uint8_t nbBits = tableLog + 1;
            uint16_t start = symbol;
            enum ReadNCountStates { PREV_IS_ZERO, REM_LT_THR, NON_ZERO_CNT };
            ReadNCountStates fsmState = NON_ZERO_CNT;
            ReadNCountStates fsmNextState = NON_ZERO_CNT;
            bool previousIs0 = false;
            bool skipZeroFreq = true;
        gen_fse_header_bitstream:
            while ((symbol < maxCharSize) && (remaining > 1)) {
#pragma HLS PIPELINE II = 1
                if (fsmState == PREV_IS_ZERO) {
                    if (skipZeroFreq) {
                        if (symbol < maxCharSize && !normTable[symbol]) {
                            ++symbol;
                        } else {
                            skipZeroFreq = false;
                        }
                    } else {
                        if (symbol >= start + 24) {
                            start += 24;
                            // bitStream += 0xFFFF << bitCount;
                            bitStream.range(15 + bitCount, bitCount) = 0xFFFF;
                            bitCount += 16;
                        } else if (symbol >= start + 3) {
                            start += 3;
                            // bitStream += 3 << bitCount;
                            bitStream.range(1 + bitCount, bitCount) = 3;
                            bitCount += 2;
                        } else {
                            fsmState = NON_ZERO_CNT;
                            // bitStream += (uint64_t)(symbol - start) << bitCount;
                            bitStream.range(1 + bitCount, bitCount) = symbol - start;
                            bitCount += 2;
                        }
                    }
                } else if (fsmState == REM_LT_THR) {
                    --nbBits;
                    threshold >>= 1;
                    if (remaining > threshold - 1) {
                        fsmState = fsmNextState;
                    }
                } else {
                    int16_t count = normTable[symbol++];
                    int max = (2 * threshold) - (1 + remaining);
                    remaining -= (count < 0) ? -count : count;
                    ++count;
                    if (count >= threshold) count += max;
                    // bitStream += count << bitCount;
                    bitStream.range(nbBits + bitCount - 1, bitCount) = count;
                    bitCount += nbBits;
                    bitCount -= (uint8_t)(count < max);
                    previousIs0 = (count == 1);
                    start = symbol;      // set starting symbol for PREV_IS_ZERO state
                    skipZeroFreq = true; // enable skipping of zero norm values for PREV_IS_ZERO state
                    fsmNextState = (previousIs0 ? PREV_IS_ZERO : NON_ZERO_CNT);
                    fsmState = ((remaining < threshold) ? REM_LT_THR : fsmNextState);
                }
                // write output bitstream 16-bits at a time
                if (bitCount > 15) {
                    outVal.data[0] = bitStream.range(7, 0);
                    outVal.data[1] = bitStream.range(15, 8);
                    // outVal.data[2] = bitStream.range(23, 16);
                    // outVal.data[3] = bitStream.range(31, 24);
                    fseHeaderStream << outVal;
                    bitStream >>= 16;
                    bitCount -= 16;
                    outCount += 2;
                }
            }
            if (bitCount) {
                outVal.strobe = (uint8_t)((bitCount + 7) / 8);
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                // outVal.data[2] = bitStream.range(23, 16);
                // outVal.data[3] = bitStream.range(31, 24);
                fseHeaderStream << outVal;
                outCount += outVal.strobe;
            }
            outVal.strobe = 0;
            fseHeaderStream << outVal;
        }
    }
    outVal.strobe = 0;
    fseHeaderStream << outVal;
}

// Content of called function
void fseEncodingTableGen(hls::stream<IntVectorStream_dt<16, 1> >& normTableStream,
                         hls::stream<IntVectorStream_dt<36, 1> >& fseTableStream) {
    // generate normalized counter to be used for fse table generation
    int16_t normTable[64];
    uint32_t intm[257];
#pragma HLS BIND_STORAGE variable = normTable type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = intm type = ram_2p impl = lutram
    uint8_t symTable[512];
    uint16_t tableU16[512];
    IntVectorStream_dt<36, 1> outVal;
    uint8_t cIdx = 0;
    while (true) {
        // read normalized counter
        auto inVal = normTableStream.read();
        if (inVal.strobe == 0) break;
        normTable[0] = inVal.data[0];
    fetg_read_norm_tbl:
        for (uint8_t i = 1; i < 64; ++i) {
#pragma HLS PIPELINE II = 1
            inVal = normTableStream.read();
            normTable[i] = inVal.data[0];
        }
        uint16_t maxSymbol = normTable[63];
        uint8_t tableLog = normTable[62];
        outVal.strobe = 1;
        // send tableLog and maxSymbol
        outVal.data[0].range(7, 0) = tableLog;
        outVal.data[0].range(35, 8) = maxSymbol;
        fseTableStream << outVal;

        if (tableLog > 0) {
            uint16_t tableSize = 1 << tableLog;
            uint32_t tableMask = tableSize - 1;
            const uint32_t step = (tableSize >> 1) + (tableSize >> 3) + 3;
            uint32_t highThreshold = tableSize - 1;

            intm[0] = 0;
            uint32_t ivSp = intm[0];
        fse_gen_symbol_start_pos:
            for (uint32_t s = 1; s <= maxSymbol + 1; ++s) {
#pragma HLS PIPELINE II = 1
                auto nvSp = normTable[s - 1];
                int16_t ivInc = 1;
                if (nvSp == -1) {
                    symTable[highThreshold] = s - 1;
                    --highThreshold;
                } else {
                    ivInc = nvSp;
                }
                intm[s] = ivSp + ivInc;
                ivSp += ivInc;
            }
            intm[maxSymbol + 1] = tableSize + 1;

            // spread symbols
            uint16_t pos = 0;
        fse_spread_symbols_outer:
            for (uint32_t s = 0; s <= maxSymbol; ++s) { // NOTE: Latency more than 1K
                auto nV = normTable[s];
            fse_spread_symbols:
                for (int16_t n = 0; n < nV;) {
#pragma HLS PIPELINE II = 1 rewind
                    if (pos < highThreshold + 1) {
                        symTable[pos] = s;
                        ++n;
                    }
                    pos = (pos + step) & tableMask;
                }
            }
        build_table:
            for (uint16_t u = 0; u < tableSize; ++u) { // NOTE: Latency 512
#pragma HLS PIPELINE II = 1
                auto s = symTable[u];
                tableU16[intm[s]++] = tableSize + u;
            }

        // send state table, tableSize on reader side can be calculated using tableLog
        send_state_table:
            for (uint16_t i = 0; i < tableSize; ++i) { // NOTE: Latency 512
#pragma HLS PIPELINE II = 1
                outVal.data[0] = tableU16[i];
                fseTableStream << outVal;
            }

            // printf("Find state and bit count table\n");
            uint16_t total = 0;
        build_sym_transformation_table:
            for (uint16_t s = 0; s <= maxSymbol; ++s) {
#pragma HLS PIPELINE II = 1
                int nv = normTable[s];
                uint8_t sym = 0;
                uint32_t nBits = 0;
                int16_t findState = 0;
                if (nv == 0) {
                    nBits = ((tableLog + 1) << 16) - (1 << tableLog);
                } else if (nv == 1 || nv == -1) {
                    nBits = (tableLog << 16) - (1 << tableLog);
                    findState = total - 1;
                    ++total;
                } else {
                    uint8_t maxBitsOut = tableLog - bitsUsed31(nv - 1);
                    uint32_t minStatePlus = (uint32_t)nv << maxBitsOut;
                    nBits = (maxBitsOut << 16) - minStatePlus;
                    findState = total - nv;
                    total += nv;
                }
                outVal.data[0].range(19, 0) = nBits;
                outVal.data[0].range(35, 20) = findState;
                fseTableStream << outVal;
            }
        }
        outVal.strobe = 0;
        fseTableStream << outVal;
        cIdx++;
        if (cIdx == 4) cIdx = 0;
    }
    outVal.strobe = 0;
    fseTableStream << outVal;
}