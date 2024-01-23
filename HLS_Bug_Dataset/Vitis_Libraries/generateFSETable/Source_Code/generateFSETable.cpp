int generateFSETable(hls::stream<uint32_t>& fseTableStream,
                     hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                     ap_uint<IN_BYTES * 8 * 2>& fseAcc,
                     uint8_t& bytesInAcc,
                     uint8_t& tableLog,
                     uint16_t maxChar,
                     xfSymbolCompMode_t fseMode,
                     xfSymbolCompMode_t prevFseMode,
                     const int16_t* defDistTable,
                     int16_t* prevDistribution) {
    const uint16_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    uint32_t bytesAvailable = bytesInAcc;
    uint8_t bitsAvailable = bytesInAcc * 8;
    uint32_t bitsConsumed = 0;
    int totalBytesConsumed = 0;
    uint32_t totalBitsConsumed = 0;
    int remaining = 0;
    int threshold = 0;
    bool previous0 = false;
    uint16_t charnum = 0;
    int16_t normalizedCounter[64];
    // initialize normalized counter
    for (int i = 0; i < 64; ++i) normalizedCounter[i] = 0;

    if (fseMode == FSE_COMPRESSED_MODE) {
        // read input bytes
        if (bytesAvailable < c_inputByte) {
            fseAcc.range((bitsAvailable + c_streamWidth - 1), bitsAvailable) = inStream.read();
            bytesAvailable += c_inputByte;
            bitsAvailable += c_streamWidth;
        }
        uint8_t accuracyLog = (fseAcc & 0xF) + 5;
        tableLog = accuracyLog;
        fseAcc >>= 4;
        bitsAvailable -= 4;
        totalBitsConsumed = 4;

        remaining = (1 << accuracyLog) + 1;
        threshold = 1 << accuracyLog;
        accuracyLog += 1;
    fsegenNormDistTable:
        while ((remaining > 1) & (charnum <= maxChar)) {
#pragma HLS PIPELINE II = 1
            bitsConsumed = 0;
            // read input bytes
            if (bytesAvailable < c_inputByte - 1) {
                fseAcc.range((bitsAvailable + c_streamWidth - 1), bitsAvailable) = inStream.read();
                bytesAvailable += c_inputByte;
                bitsAvailable += c_streamWidth;
            }
            if (previous0) {
                unsigned n0 = 0;
                if ((fseAcc & 0xFFFF) == 0xFFFF) {
                    n0 += 24;
                    bitsConsumed += 16;
                } else if ((fseAcc & 3) == 3) {
                    n0 += 3;
                    bitsConsumed += 2;
                } else {
                    n0 += fseAcc & 3;
                    bitsConsumed += 2;
                    previous0 = false;
                }
                charnum += n0;
            } else {
                int16_t max = (2 * threshold - 1) - remaining;
                int count;
                uint8_t c1 = 0;
                if ((fseAcc & (threshold - 1)) < max) {
                    c1 = 1;
                    count = fseAcc & (threshold - 1);
                    bitsConsumed += accuracyLog - 1;
                } else {
                    c1 = 2;
                    count = fseAcc & (2 * threshold - 1);
                    if (count >= threshold) count -= max;
                    bitsConsumed += accuracyLog;
                }

                --count;                                     /* extra accuracy */
                remaining -= ((count < 0) ? -count : count); /* -1 means +1 */
                normalizedCounter[charnum++] = count;
                previous0 = ((count == 0) ? 1 : 0);
            genDTableIntLoop:
                while (remaining < threshold) {
                    --accuracyLog;
                    threshold >>= 1;
                }
            }
            fseAcc >>= bitsConsumed;
            bitsAvailable -= bitsConsumed;
            totalBitsConsumed += bitsConsumed;

            bytesAvailable = bitsAvailable >> 3;
        }
        totalBytesConsumed += (totalBitsConsumed + 7) >> 3;
        bytesInAcc = bytesAvailable;
        // clear accumulator, so that it is byte aligned
        fseAcc >>= (bitsAvailable & 7);
        // copy to prevDistribution table
        for (int i = 0; i < 64; ++i) prevDistribution[i] = normalizedCounter[i];
    } else if (fseMode == PREDEFINED_MODE) { /*TODO: use fixed table directly*/
        for (int i = 0; i < maxChar + 1; ++i) {
            normalizedCounter[i] = defDistTable[i];
            prevDistribution[i] = defDistTable[i];
        }
        charnum = maxChar + 1;
    } else if (fseMode == RLE_MODE) {
        // next state for entire table is 0
        // accuracy log is 0
        // read input bytes
        if (bytesAvailable < 1) {
            fseAcc.range((bitsAvailable + c_streamWidth - 1), bitsAvailable) = inStream.read();
            bytesAvailable += c_inputByte;
            bitsAvailable += c_streamWidth;
        }
        uint8_t symbol = (uint8_t)fseAcc;
        fseAcc >>= 8;
        bytesInAcc = bytesAvailable - 1;

        fseTableStream << 1; // tableSize
        fseTableStream << symbol;
        tableLog = 0;

        return 1;
    } else if (fseMode == REPEAT_MODE) {
        // check if previous mode was RLE
        fseTableStream << 0xFFFFFFFF; // all 1's to indicate use previous table
        return 0;
    } else {
        // else -> Error: invalid fse mode
        return 0;
    }

    // Table Generation
    const uint32_t tableSize = 1 << tableLog;
    uint32_t highThreshold = tableSize - 1;
    uint16_t symbolNext[64];
    int16_t symTable[513];
    // initialize table
    for (uint32_t i = 0; i < tableSize; ++i) symTable[i] = 0;

fse_gen_next_symbol_table:
    for (uint16_t i = 0; i < charnum; i++) {
#pragma HLS PIPELINE II = 1
        if (normalizedCounter[i] == -1) {
            symTable[highThreshold] = i; // symbol, assign lower 8-bits
            --highThreshold;
            symbolNext[i] = 1;
        } else {
            symbolNext[i] = normalizedCounter[i];
        }
    }

    uint32_t mask = tableSize - 1;
    const uint32_t step = (tableSize >> 1) + (tableSize >> 3) + 3;
    uint32_t pos = 0;

fse_gen_symbol_table_outer:
    for (uint16_t i = 0; i < charnum; ++i) {
    fse_gen_symbol_table:
        for (int j = 0; j < normalizedCounter[i];) {
#pragma HLS PIPELINE II = 1
            if (pos > highThreshold) {
                pos = (pos + step) & mask;
            } else {
                symTable[pos] = i;
                pos = (pos + step) & mask;
                ++j;
            }
        }
    }
    // FSE table
    fseTableStream << tableSize;
gen_fse_final_table:
    for (uint32_t i = 0; i < tableSize; i++) {
#pragma HLS PIPELINE II = 1
        uint8_t symbol = (uint8_t)symTable[i];
        uint16_t nextState = symbolNext[symbol]++;
        uint16_t nBits = (uint8_t)(tableLog - (31 - __builtin_clz(nextState)));
        uint32_t tblVal = ((uint32_t)((nextState << nBits) - tableSize) << 16) + ((uint32_t)nBits << 8) + symbol;
        fseTableStream << tblVal;
    }

    return totalBytesConsumed;
}