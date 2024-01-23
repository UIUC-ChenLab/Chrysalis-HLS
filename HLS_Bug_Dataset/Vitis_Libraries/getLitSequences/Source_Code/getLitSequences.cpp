void getLitSequences(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                     hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                     hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                     hls::stream<bool>& rleFlagStream,
                     hls::stream<ap_uint<2> >& strdBlockFlagStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outMetaStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > metaStream("metaStream");
#pragma HLS STREAM variable = compressedStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = metaStream depth = 8

    // LZ77 compress
    xf::compression::lzCompress<BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(inStream,
                                                                                                 compressedStream);
    // improve CR and generate clean sequences
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    // separate literals from sequences and generate literal frequencies
    xf::compression::details::zstdLz77DivideStream<MAX_FREQ_DWIDTH, MIN_MATCH>(
        boosterStream, litStream, seqStream, litFreqStream, seqFreqStream, rleFlagStream, metaStream, litCntStream);
    xf::compression::details::zstdCompressionMeta<BLOCK_SIZE, MAX_FREQ_DWIDTH>(metaStream, strdBlockFlagStream,
                                                                               outMetaStream);
}

// Content of called function
void lzCompress(hls::stream<IntVectorStream_dt<8, 1> >& inStream, hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    const uint16_t c_indxBitCnts = 24;
    const uint16_t c_fifo_depth = LEFT_BYTES + 2;
    const int c_dictEleWidth = (MATCH_LEN * 8 + c_indxBitCnts);
    typedef ap_uint<MATCH_LEVEL * c_dictEleWidth> uintDictV_t;
    typedef ap_uint<c_dictEleWidth> uintDict_t;
    const uint32_t totalDictSize = (1 << (c_indxBitCnts - 1)); // 8MB based on index 3 bytes
#ifndef AVOID_STATIC_MODE
    static bool resetDictFlag = true;
    static uint32_t relativeNumBlocks = 0;
#else
    bool resetDictFlag = true;
    uint32_t relativeNumBlocks = 0;
#endif

    uintDictV_t dict[LZ_DICT_SIZE];
#pragma HLS RESOURCE variable = dict core = XPM_MEMORY uram

    // local buffers for each block
    uint8_t present_window[MATCH_LEN];
#pragma HLS ARRAY_PARTITION variable = present_window complete
    hls::stream<uint8_t> lclBufStream("lclBufStream");
#pragma HLS STREAM variable = lclBufStream depth = c_fifo_depth
#pragma HLS BIND_STORAGE variable = lclBufStream type = fifo impl = srl

    // input register
    IntVectorStream_dt<8, 1> inVal;
    // output register
    IntVectorStream_dt<32, 1> outValue;
    // loop over blocks
    while (true) {
        uint32_t iIdx = 0;
        // once 8MB data is processed reset dictionary
        // 8MB based on index 3 bytes
        if (resetDictFlag) {
            ap_uint<MATCH_LEVEL* c_dictEleWidth> resetValue = 0;
            for (int i = 0; i < MATCH_LEVEL; i++) {
#pragma HLS UNROLL
                resetValue.range((i + 1) * c_dictEleWidth - 1, i * c_dictEleWidth + MATCH_LEN * 8) = -1;
            }
        // Initialization of Dictionary
        dict_flush:
            for (int i = 0; i < LZ_DICT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
                dict[i] = resetValue;
            }
            resetDictFlag = false;
            relativeNumBlocks = 0;
        } else {
            relativeNumBlocks++;
        }
        // check if end of data
        auto nextVal = inStream.read();
        if (nextVal.strobe == 0) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }
    // fill buffer and present_window
    lz_fill_present_win:
        while (iIdx < MATCH_LEN - 1) {
#pragma HLS PIPELINE II = 1
            inVal = nextVal;
            nextVal = inStream.read();
            present_window[++iIdx] = inVal.data[0];
        }
    // assuming that, at least bytes more than LEFT_BYTES will be present at the input
    lz_fill_circular_buf:
        for (uint16_t i = 0; i < LEFT_BYTES; ++i) {
#pragma HLS PIPELINE II = 1
            inVal = nextVal;
            nextVal = inStream.read();
            lclBufStream << inVal.data[0];
        }
        // lz_compress main
        outValue.strobe = 1;

    lz_compress:
        for (; nextVal.strobe != 0; ++iIdx) {
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = dict inter false
#endif
            uint32_t currIdx = (iIdx + (relativeNumBlocks * MAX_INPUT_SIZE)) - MATCH_LEN + 1;
            // read from input stream into circular buffer
            auto inValue = lclBufStream.read(); // pop latest value from FIFO
            lclBufStream << nextVal.data[0];    // push latest read value to FIFO
            nextVal = inStream.read();          // read next value from input stream

            // shift present window and load next value
            for (uint8_t m = 0; m < MATCH_LEN - 1; m++) {
#pragma HLS UNROLL
                present_window[m] = present_window[m + 1];
            }

            present_window[MATCH_LEN - 1] = inValue;

            // Calculate Hash Value
            uint32_t hash = 0;
            if (MIN_MATCH == 3) {
                hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^
                       (present_window[0] << 1) ^ (present_window[1]);
            } else {
                hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^
                       (present_window[3]);
            }

            // Dictionary Lookup
            uintDictV_t dictReadValue = dict[hash];
            uintDictV_t dictWriteValue = dictReadValue << c_dictEleWidth;
            for (int m = 0; m < MATCH_LEN; m++) {
#pragma HLS UNROLL
                dictWriteValue.range((m + 1) * 8 - 1, m * 8) = present_window[m];
            }
            dictWriteValue.range(c_dictEleWidth - 1, MATCH_LEN * 8) = currIdx;
            // Dictionary Update
            dict[hash] = dictWriteValue;

            // Match search and Filtering
            // Comp dict pick
            uint8_t match_length = 0;
            uint32_t match_offset = 0;
            for (int l = 0; l < MATCH_LEVEL; l++) {
                uint8_t len = 0;
                bool done = 0;
                uintDict_t compareWith = dictReadValue.range((l + 1) * c_dictEleWidth - 1, l * c_dictEleWidth);
                uint32_t compareIdx = compareWith.range(c_dictEleWidth - 1, MATCH_LEN * 8);
                for (uint8_t m = 0; m < MATCH_LEN; m++) {
                    if (present_window[m] == compareWith.range((m + 1) * 8 - 1, m * 8) && !done) {
                        len++;
                    } else {
                        done = 1;
                    }
                }
                if ((len >= MIN_MATCH) && (currIdx > compareIdx) && ((currIdx - compareIdx) < LZ_MAX_OFFSET_LIMIT) &&
                    ((currIdx - compareIdx - 1) >= MIN_OFFSET) &&
                    (compareIdx >= (relativeNumBlocks * MAX_INPUT_SIZE))) {
                    if ((len == 3) && ((currIdx - compareIdx - 1) > 4096)) {
                        len = 0;
                    }
                } else {
                    len = 0;
                }
                if (len > match_length) {
                    match_length = len;
                    match_offset = currIdx - compareIdx - 1;
                }
            }
            outValue.data[0].range(7, 0) = present_window[0];
            outValue.data[0].range(15, 8) = match_length;
            outValue.data[0].range(31, 16) = match_offset;
            outStream << outValue;
        }

        outValue.data[0] = 0;
    lz_compress_leftover:
        for (uint8_t m = 1; m < MATCH_LEN; ++m) {
#pragma HLS PIPELINE II = 1
            outValue.data[0].range(7, 0) = present_window[m];
            outStream << outValue;
        }
    lz_left_bytes:
        for (uint16_t l = 0; l < LEFT_BYTES; ++l) {
#pragma HLS PIPELINE II = 1
            outValue.data[0].range(7, 0) = lclBufStream.read();
            outStream << outValue;
        }

        // once relativeInSize becomes 8MB set the flag to true
        resetDictFlag = ((relativeNumBlocks * MAX_INPUT_SIZE) >= (totalDictSize)) ? true : false;
        // end of block
        outValue.strobe = 0;
        outStream << outValue;
    }
}

// Content of called function
void zstdLz77DivideStream(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                          hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                          hls::stream<bool>& rleFlagStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                          hls::stream<FREQ_DT>& litCntStream) {
    // lz77 encoder states
    enum LZ77EncoderStates { WRITE_LITERAL, WRITE_OFFSET0, WRITE_OFFSET1 };
    IntVectorStream_dt<8, 1> outLitVal;
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outLitFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outSeqFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> metaVal;
    metaVal.strobe = 1;
    // frequency buffers
    FREQ_DT literal_freq[c_maxLitV + 1];
    FREQ_DT litlen_freq[c_maxCodeLL + 1];
    FREQ_DT matlen_freq[c_maxCodeML + 1];
    FREQ_DT offset_freq[c_maxCodeOF + 1];
#pragma HLS BIND_STORAGE variable = literal_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = litlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = matlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = offset_freq type = ram_2p impl = lutram

    bool last_block = false;
    bool just_started = true;
    int blk_n = 0;

    while (!last_block) {
        // iterate over multiple blocks in a file
        ++blk_n;
        enum LZ77EncoderStates next_state = WRITE_LITERAL;
        FREQ_DT seqCnt = 0;

        { // Initialize frequencies memory
#pragma HLS LOOP_MERGE force
        lit_freq_init:
            for (uint16_t i = 0; i < c_maxLitV + 1; i++) {
                literal_freq[i] = 0;
            }
        ll_freq_init:
            for (uint16_t i = 0; i < c_maxCodeLL + 1; i++) {
                litlen_freq[i] = 0;
            }
        ml_freq_init:
            for (uint16_t i = 0; i < c_maxCodeML + 1; i++) {
                matlen_freq[i] = 0;
            }
        of_freq_init:
            for (uint16_t i = 0; i < c_maxCodeOF + 1; i++) {
                offset_freq[i] = 0;
            }
        }
        uint8_t tCh = 0;
        uint8_t tLen = 0;
        FREQ_DT litCount = 0;
        FREQ_DT litTotal = 0;
        // set output data to be of valid length
        outLitVal.strobe = 1;
        outSeqVal.strobe = 1;
        uint8_t cLit = 0;
        bool fv = true;
        bool isRLE = true;
    zstd_lz77_divide:
        while (true) {
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = literal_freq inter false
#pragma HLS dependence variable = litlen_freq inter false
#pragma HLS dependence variable = matlen_freq inter false
#pragma HLS dependence variable = offset_freq inter false
#endif
            // read value from stream
            auto encodedValue = inStream.read();
            if (encodedValue.strobe == 0) {
                last_block = just_started;
                just_started = true;
                break;
            }
            just_started = false;
            tCh = encodedValue.data[0].range(7, 0);
            tLen = encodedValue.data[0].range(15, 8);
            uint16_t tOffset = encodedValue.data[0].range(31, 16) + 3 + 1;

            if (tLen) {
                // if match length present, get sequence codes
                uint8_t llc = getLLCode<16>((ap_uint<16>)litCount);
                uint8_t mlc = getMLCode<8>((ap_uint<8>)(tLen - MIN_MATCH));
                uint8_t ofc = bitsUsed31((uint32_t)tOffset);
                // reset code frequencies
                litlen_freq[llc]++;
                matlen_freq[mlc]++;
                offset_freq[ofc]++;
                // write sequence
                outSeqVal.data[0].litlen = litCount;
                outSeqVal.data[0].matlen = tLen; // - MIN_MATCH;
                outSeqVal.data[0].offset = tOffset;
                seqStream << outSeqVal;
                litCount = 0;
                ++seqCnt;
            } else {
                // store first literal to check for RLE block
                if (fv) {
                    cLit = tCh;
                } else {
                    if (cLit != tCh) isRLE = false;
                }
                fv = false;
                // increment literal count
                literal_freq[tCh]++;
                ++litCount;
                ++litTotal;
                // write literal
                outLitVal.data[0] = tCh;
                litStream << outLitVal;
            }
        }
        if (!last_block) {
            isRLE = (isRLE && cLit == 0);
            if (isRLE) {
                // printf("RLE literals\n");
                literal_freq[3]++;
                outLitVal.data[0] = 3;
                litStream << outLitVal;
            }
            rleFlagStream << isRLE;
        }
        // fix for zero sequences
        if (!last_block && seqCnt == 0) {
            outSeqVal.data[0].litlen = 0;
            outSeqVal.data[0].matlen = 0;
            outSeqVal.data[0].offset = 0;
            seqStream << outSeqVal;
        }

        // write strobe = 0
        outLitVal.strobe = 0;
        outSeqVal.strobe = 0;
        litStream << outLitVal;
        seqStream << outSeqVal;
        // write literal and distance trees
        if (!last_block) {
            metaVal.data[0] = litTotal;
            metaStream << metaVal;
            metaVal.data[0] = seqCnt;
            metaStream << metaVal;
            litCntStream << (isRLE ? (FREQ_DT)(litTotal + 1) : litTotal);
            // printf("litCount: %u, seqCnt: %u\n", (uint16_t)litTotal, (uint16_t)seqCnt);
            outLitFreqVal.strobe = 1;
            outSeqFreqVal.strobe = 1;
        write_lit_freq:
            for (ap_uint<9> i = 0; i < c_maxLitV + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outLitFreqVal.data[0] = literal_freq[i];
                litFreqStream << outLitFreqVal;
            }
            // first write total number of sequences
            outSeqFreqVal.data[0] = seqCnt;
            seqFreqStream << outSeqFreqVal;
            // write last ll, ml, of codes
            outSeqFreqVal.data[0] = getLLCode<16>(outSeqVal.data[0].litlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = getMLCode<8>(outSeqVal.data[0].matlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = bitsUsed31(outSeqVal.data[0].offset);
            seqFreqStream << outSeqFreqVal;
        write_lln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeLL + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = litlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_ofs_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeOF + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = offset_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_mln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeML + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = matlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        }
    }
    // eos needed only to indicated end of block
    outLitFreqVal.strobe = 0;
    outSeqFreqVal.strobe = 0;
    metaVal.strobe = 0;
    litFreqStream << outLitFreqVal;
    seqFreqStream << outSeqFreqVal;
    metaStream << metaVal;
}

// Content of called function
void lzBooster(hls::stream<compressd_dt>& inStream, hls::stream<compressd_dt>& outStream, uint32_t input_size) {
    if (input_size == 0) return;
    uint8_t local_mem[BOOSTER_OFFSET_WINDOW];
    uint32_t match_loc = 0;
    uint32_t match_len = 0;
    compressd_dt outValue;
    compressd_dt outStreamValue;
    bool matchFlag = false;
    bool outFlag = false;
    bool boostFlag = false;
    uint16_t skip_len = 0;
    uint8_t nextMatchCh = local_mem[match_loc % BOOSTER_OFFSET_WINDOW];

lz_booster:
    for (uint32_t i = 0; i < (input_size - LEFT_BYTES); i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = local_mem inter false
        compressd_dt inValue = inStream.read();
        uint8_t tCh = inValue.range(7, 0);
        uint8_t tLen = inValue.range(15, 8);
        uint16_t tOffset = inValue.range(31, 16);
        if (tOffset < BOOSTER_OFFSET_WINDOW) {
            boostFlag = true;
        } else {
            boostFlag = false;
        }
        uint8_t match_ch = nextMatchCh;
        local_mem[i % BOOSTER_OFFSET_WINDOW] = tCh;
        outFlag = false;

        if (skip_len) {
            skip_len--;
        } else if (matchFlag && (match_len < MAX_MATCH_LEN) && (tCh == match_ch)) {
            match_len++;
            match_loc++;
            outValue.range(15, 8) = match_len;
        } else {
            match_len = 1;
            match_loc = i - tOffset;
            if (i) outFlag = true;
            outStreamValue = outValue;
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
        nextMatchCh = local_mem[match_loc % BOOSTER_OFFSET_WINDOW];
        if (outFlag) outStream << outStreamValue;
    }
    outStream << outValue;
lz_booster_left_bytes:
    for (uint32_t i = 0; i < LEFT_BYTES; i++) {
#pragma HLS PIPELINE off
        outStream << inStream.read();
    }
}

// Content of called function
void zstdCompressionMeta(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                         hls::stream<ap_uint<2> >& strdBlockFlagStream,
                         hls::stream<DT>& outMetaStream) {
    uint8_t i = 0;
    DT litCnt = 0;
    ap_uint<2> stbFVal = 1; // <stb Flag 1-bit><strobe 1-bit>
gen_meta_loop:
    for (auto inMetaVal = metaStream.read(); inMetaVal.strobe > 0; inMetaVal = metaStream.read()) {
#pragma HLS PIPELINE off
        litCnt = inMetaVal.data[0];
        outMetaStream << litCnt;
        if (i == 0) {
            stbFVal.range(1, 1) = (ap_int<1>)(litCnt > BLOCK_SIZE - 2048);
            strdBlockFlagStream << stbFVal;
        }
        i = (i + 1) & 1;
    }
    // end of all data
    stbFVal = 0;
    strdBlockFlagStream << stbFVal;
}