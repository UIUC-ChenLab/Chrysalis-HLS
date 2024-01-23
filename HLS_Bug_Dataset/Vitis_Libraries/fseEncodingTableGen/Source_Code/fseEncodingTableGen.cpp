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