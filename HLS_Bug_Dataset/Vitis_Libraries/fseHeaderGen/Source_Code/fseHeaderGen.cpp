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