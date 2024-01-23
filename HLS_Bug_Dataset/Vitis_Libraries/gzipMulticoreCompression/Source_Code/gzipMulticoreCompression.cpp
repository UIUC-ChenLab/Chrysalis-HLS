void gzipMulticoreCompression(hls::stream<ap_uint<64> >& inStream,
                              hls::stream<uint32_t>& inSizeStream,
                              hls::stream<ap_uint<32> >& checksumInitStream,
                              hls::stream<ap_uint<64> >& outStream,
                              hls::stream<bool>& outStreamEos,
                              hls::stream<uint32_t>& outSizeStream,
                              hls::stream<ap_uint<32> >& checksumOutStream,
                              hls::stream<ap_uint<2> >& checksumTypeStream) {
#pragma HLS dataflow
    constexpr int c_blockSizeInKb = BLOCK_SIZE_IN_KB;
    constexpr int c_blockSize = c_blockSizeInKb * 1024;
    constexpr int c_numBlocks = NUM_BLOCKS;
    constexpr int c_twiceNumBlocks = 2 * NUM_BLOCKS;
    constexpr int c_blckEosDepth = (NUM_BLOCKS * NUM_BLOCKS) + NUM_BLOCKS;
    constexpr int c_defaultDepth = 4;
    constexpr int c_dwidth = 64;
    constexpr int c_maxBlockSize = 32768;
    constexpr int c_minBlockSize = 64;
    constexpr int c_checksumParallelBytes = 8;
    constexpr int c_bufferFifoDepth = c_blockSize / 8;
    constexpr int c_freq_dwidth = getDataPortWidth(c_blockSize);
    constexpr int c_size_dwidth = getDataPortWidth(c_checksumParallelBytes);
    constexpr int c_strdBlockDepth = c_minBlockSize / c_checksumParallelBytes;

    // Assertion for Maximum Supported Parallel Cores
    assert(NUM_BLOCKS == 1 | NUM_BLOCKS == 2 | NUM_BLOCKS == 4 | NUM_BLOCKS == 8);

    hls::stream<ap_uint<c_dwidth> > checksumStream("checksumStream");
    hls::stream<ap_uint<5> > checksumSizeStream("checksumSizeStream");
    hls::stream<ap_uint<c_dwidth> > coreStream("coreStream");
    hls::stream<uint32_t> coreSizeStream("coreSizeStream");
    hls::stream<ap_uint<c_dwidth> > distStream[c_numBlocks];
    hls::stream<ap_uint<c_freq_dwidth> > distSizeStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth> > strdStream;
    hls::stream<ap_uint<16> > strdSizeStream;
    hls::stream<ap_uint<17> > upsizedCntr[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 1> > downStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<9, 1> > lz77Stream[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 2> > huffStream[c_numBlocks];
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[c_numBlocks];
    hls::stream<ap_uint<9> > lz77PassStream[c_numBlocks]; // 9 bits
    hls::stream<IntVectorStream_dt<9, 1> > lz77DownsizedStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lz77Tree[c_numBlocks];
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > mergeStream[c_numBlocks];

#pragma HLS STREAM variable = checksumStream depth = c_defaultDepth
#pragma HLS STREAM variable = checksumSizeStream depth = c_defaultDepth
#pragma HLS STREAM variable = coreStream depth = c_defaultDepth
#pragma HLS STREAM variable = coreSizeStream depth = c_defaultDepth

#pragma HLS STREAM variable = distSizeStream depth = c_numBlocks
#pragma HLS STREAM variable = downStream depth = c_numBlocks
#pragma HLS STREAM variable = huffStream depth = c_numBlocks
#pragma HLS STREAM variable = lz77Tree depth = c_numBlocks
#pragma HLS STREAM variable = hufCodeStream depth = c_numBlocks
#pragma HLS STREAM variable = upsizedCntr depth = c_numBlocks
#pragma HLS STREAM variable = lz77DownsizedStream depth = c_numBlocks
#pragma HLS STREAM variable = lz77Stream depth = c_numBlocks

#pragma HLS STREAM variable = strdSizeStream depth = c_twiceNumBlocks
#pragma HLS STREAM variable = strdStream depth = c_strdBlockDepth
#pragma HLS STREAM variable = mergeStream depth = c_bufferFifoDepth
#pragma HLS STREAM variable = distStream depth = c_bufferFifoDepth
#pragma HLS STREAM variable = lz77PassStream depth = c_blockSize

#pragma HLS BIND_STORAGE variable = lz77Tree type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lz77PassStream type = FIFO impl = BRAM

    if (BLOCK_SIZE_IN_KB >= 32) {
#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = URAM
    } else {
#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = BRAM
    }

    // send input data to both checksum and for compression
    xf::compression::details::dataDuplicator<c_dwidth, c_checksumParallelBytes>(
        inStream, inSizeStream, checksumStream, checksumSizeStream, coreStream, coreSizeStream);

    // checksum kernel
    xf::compression::checksum32<c_checksumParallelBytes>(checksumInitStream, checksumStream, checksumSizeStream,
                                                         checksumOutStream, checksumTypeStream);

    // distrubute block size data into each block in round-robin fashion
    details::multicoreDistributor<c_freq_dwidth, c_dwidth, c_numBlocks, c_blockSize>(
        coreStream, coreSizeStream, strdStream, strdSizeStream, distStream, distSizeStream);

    // Parallel Buffers
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::streamDownSizerSize<c_dwidth, 8, c_freq_dwidth>(distStream[i], distSizeStream[i],
                                                                                  downStream[i]);
    }

    // Parallel LZ77 Compress
    lz77Compress<0, c_blockSize, c_freq_dwidth>(downStream[0], lz77Stream[0], lz77Tree[0]);
    if (NUM_BLOCKS >= 2) {
        lz77Compress<1, c_blockSize, c_freq_dwidth>(downStream[1], lz77Stream[1], lz77Tree[1]);
    }
    if (NUM_BLOCKS >= 4) {
        lz77Compress<2, c_blockSize, c_freq_dwidth>(downStream[2], lz77Stream[2], lz77Tree[2]);
        lz77Compress<3, c_blockSize, c_freq_dwidth>(downStream[3], lz77Stream[3], lz77Tree[3]);
    }
    if (NUM_BLOCKS >= 8) {
        lz77Compress<4, c_blockSize, c_freq_dwidth>(downStream[4], lz77Stream[4], lz77Tree[4]);
        lz77Compress<5, c_blockSize, c_freq_dwidth>(downStream[5], lz77Stream[5], lz77Tree[5]);
        lz77Compress<6, c_blockSize, c_freq_dwidth>(downStream[6], lz77Stream[6], lz77Tree[6]);
        lz77Compress<7, c_blockSize, c_freq_dwidth>(downStream[7], lz77Stream[7], lz77Tree[7]);
    }

    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::sendBuffer<9>(lz77Stream[i], lz77PassStream[i], upsizedCntr[i]);
    }

    // Single Call Treegen
    details::zlibTreegenStreamWrapper<c_numBlocks>(lz77Tree, hufCodeStream);

    // Parallel Huffman
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::receiveBuffer<9>(lz77PassStream[i], lz77DownsizedStream[i], upsizedCntr[i]);

        zlibHuffmanEncoder(lz77DownsizedStream[i], hufCodeStream[i], huffStream[i]);

        xf::compression::details::simpleStreamUpsizer<16, c_dwidth, c_size_dwidth>(huffStream[i], mergeStream[i]);
    }

    // read all num block data in round-robin fashion and write into single outstream
    details::multicoreMerger<c_dwidth, c_size_dwidth, c_numBlocks, c_blockSize>(mergeStream, strdStream, strdSizeStream,
                                                                                outStream, outStreamEos, outSizeStream);
}

// Content of called function
void streamDownSizerSize(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                         hls::stream<ap_uint<SIZE_DWIDTH> >& dataSizeStream,
                         hls::stream<IntVectorStream_dt<OUT_DATAWIDTH, 1> >& outStream) {
    constexpr int16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr int16_t c_outWord = OUT_DATAWIDTH / 8;
    ap_uint<IN_DATAWIDTH> inVal;
    IntVectorStream_dt<OUT_DATAWIDTH, 1> outVal;
    ap_uint<SIZE_DWIDTH> inSize = 0;

downsizer_top:
    for (auto inSize = dataSizeStream.read(); inSize > 0; inSize = dataSizeStream.read()) {
        auto outSizeV = ((inSize - 1) / c_outWord) + 1;
        outVal.strobe = 1;
    downsizer_assign:
        for (ap_uint<SIZE_DWIDTH> dsize = 0; dsize < outSizeV; ++dsize) {
#pragma HLS PIPELINE II = 1
            auto idx = dsize % c_factor;
            if (idx == 0) {
                inVal = inStream.read();
            }
            outVal.data[0] = inVal.range(OUT_DATAWIDTH - 1, 0);
            inVal >>= OUT_DATAWIDTH;
            outStream << outVal;
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}

// Content of called function
void checksum32(hls::stream<ap_uint<32> >& checksumInitStrm,
                hls::stream<ap_uint<8 * W> >& inStrm,
                hls::stream<ap_uint<5> >& inLenStrm,
                hls::stream<ap_uint<32> >& outStrm,
                hls::stream<ap_uint<2> >& checksumTypeStrm) {
    // Internal EOS Streams
    hls::stream<bool> endInStrm;
    hls::stream<bool> endOutStrm;
#pragma HLS STREAM variable = endInStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4

checksum_loop:
    for (ap_uint<2> checksumType = checksumTypeStrm.read(); checksumType != 3; checksumType = checksumTypeStrm.read()) {
        endInStrm << false;
        endInStrm << true;
        // CRC
        if (checksumType == 1) {
            xf::security::crc32<W>(checksumInitStrm, inStrm, inLenStrm, endInStrm, outStrm, endOutStrm);
        }
        // ADLER
        else {
            xf::security::adler32<W>(checksumInitStrm, inStrm, inLenStrm, endInStrm, outStrm, endOutStrm);
        }
        endOutStrm.read();
        endOutStrm.read();
    }
}

// Content of called function
void adler32(hls::stream<ap_uint<W * 8> >& inStrm,
             hls::stream<ap_uint<5> >& inPackLenStrm,
             hls::stream<ap_uint<32> >& outStrm) {
    // To be deprecated
    hls::stream<bool> endInPackLenStrm;
    hls::stream<bool> endOutStrm;
    hls::stream<ap_uint<32> > adlerStrm;
#pragma HLS STREAM variable = endInPackLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4
#pragma HLS STREAM variable = adlerStrm depth = 4

    for (int i = 0; i < NUMCORES; i++) {
        endInPackLenStrm << false;
        endInPackLenStrm << true;
        adlerStrm << 1;
        xf::security::adler32<W>(adlerStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
        endOutStrm.read();
        endOutStrm.read();
    }
}

// Content of called function
void crc32(hls::stream<ap_uint<32> >& crcInitStrm,
           hls::stream<ap_uint<8 * W> >& inStrm,
           hls::stream<ap_uint<5> >& inPackLenStrm,
           hls::stream<ap_uint<32> >& outStrm) {
    hls::stream<bool> endInPackLenStrm;
    hls::stream<bool> endOutStrm;
#pragma HLS STREAM variable = endInPackLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4
    endInPackLenStrm << false;
    endInPackLenStrm << true;
    xf::security::crc32<W>(crcInitStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
    endOutStrm.read();
    endOutStrm.read();
}

// Content of called function
void lz77Compress(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                  hls::stream<IntVectorStream_dt<9, 1> >& lz77OutStream,
                  hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outTreeStream) {
    constexpr int c_litDistCodeDepth = 286 + 30 + 4;
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream1("compressedStream1");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
#pragma HLS STREAM variable = compressedStream depth = 4
#pragma HLS STREAM variable = compressedStream1 depth = 4
#pragma HLS STREAM variable = boosterStream depth = c_litDistCodeDepth
#pragma HLS BIND_STORAGE variable = boosterStream type = FIFO impl = SRL

    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_ID>(
        inStream, compressedStream);
    xf::compression::lzBestMatchFilter<MATCH_LEN, LZ_MAX_OFFSET_LIMIT>(compressedStream, compressedStream1);
    xf::compression::lzBooster<MAX_MATCH_LEN, MAX_BLOCK_SIZE>(compressedStream1, boosterStream);
    xf::compression::lz77DivideStream<MAX_FREQ_DWIDTH, CORE_ID>(boosterStream, lz77OutStream, outTreeStream);
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
void lzBestMatchFilter(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                       hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    const int c_max_match_length = MATCH_LEN;

    IntVectorStream_dt<32, 1> compare_window;
    IntVectorStream_dt<32, 1> outStreamValue;

    while (true) {
        auto nextVal = inStream.read();
        if (nextVal.strobe == 0) {
            outStreamValue.strobe = 0;
            outStream << outStreamValue;
            break;
        }
        // assuming that, at least bytes more than LEFT_BYTES will be present at the input
        compare_window = nextVal;
        nextVal = inStream.read();

    lz_bestMatchFilter:
        for (; nextVal.strobe != 0;) {
#pragma HLS PIPELINE II = 1
            // shift register logic
            IntVectorStream_dt<32, 1> outValue = compare_window;
            compare_window = nextVal;
            nextVal = inStream.read();

            uint8_t match_length = outValue.data[0].range(15, 8);
            bool best_match = 1;
            // Find Best match
            compressd_dt compareValue = compare_window.data[0];
            uint8_t compareLen = compareValue.range(15, 8);
            if (match_length < compareLen) {
                best_match = 0;
            }
            if (best_match == 0) {
                outValue.data[0].range(15, 8) = 0;
                outValue.data[0].range(31, 16) = 0;
            }
            outStream << outValue;
        }
        outStream << compare_window;
        outStreamValue.strobe = 0;
        outStream << outStreamValue;
    }
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
void zlibHuffmanEncoder(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                        hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeInStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& huffOutStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > encodedOutStream("encodedOutStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream("hufCodeStream");
#pragma HLS STREAM variable = encodedOutStream depth = 4
#pragma HLS STREAM variable = hufCodeStream depth = 4

    xf::compression::details::huffmanProcessingUnit(inStream, encodedOutStream);
    xf::compression::huffmanEncoderStream(encodedOutStream, hufCodeInStream, hufCodeStream);
    xf::compression::details::bitPackingStream(hufCodeStream, huffOutStream);
}

// Content of called function
void huffmanEncoderStream(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeInStream,
                          hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeOutStream) {
    enum huffmanEncoderState { WRITE_TOKEN, ML_DIST_REP, LIT_REP, SEND_OUTPUT, ML_EXTRA, DIST_REP, DIST_EXTRA, DONE };
    ap_uint<4> extra_lbits[c_length_codes] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                              2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    ap_uint<4> extra_dbits[c_distance_codes] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                                6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    const uint16_t c_ltree_size = 286;
    const uint8_t c_dtree_size = 30;

    bool done = false;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outValue;

    while (true) {
        HuffmanCode_dt<c_maxBits> litlnTree[c_ltree_size];
        HuffmanCode_dt<c_maxBits> distTree[c_dtree_size];
#pragma HLS aggregate variable = litlnTree
#pragma HLS aggregate variable = distTree

        // check if any tree data present
        auto hfcIn = hufCodeInStream.read();
        if (hfcIn.strobe == 0) {
            inStream.read(); // read zero strobe from input
            break;
        }
    init_litlen_tree_table:
        for (ap_uint<9> i = 0; i < c_ltree_size - 1; ++i) {
#pragma HLS PIPELINE II = 1
            litlnTree[i].code = hfcIn.data[0].code;
            litlnTree[i].bitlen = hfcIn.data[0].bitlen;
            hfcIn = hufCodeInStream.read();
        }
        litlnTree[c_ltree_size - 1].code = hfcIn.data[0].code;
        litlnTree[c_ltree_size - 1].bitlen = hfcIn.data[0].bitlen;

    init_dist_tree_table:
        for (ap_uint<5> i = 0; i < c_dtree_size; ++i) {
#pragma HLS PIPELINE II = 1
            hfcIn = hufCodeInStream.read();
            distTree[i].code = hfcIn.data[0].code;
            distTree[i].bitlen = hfcIn.data[0].bitlen;
        }

    // Passthrough the Preamble Data coming from Treegen to BitPacker
    preamble_pass_through:
        for (hfcIn = hufCodeInStream.read(); hfcIn.data[0].bitlen != 0; hfcIn = hufCodeInStream.read()) {
#pragma HLS PIPELINE II = 1
            hufCodeOutStream << hfcIn;
        }

        enum huffmanEncoderState next_state = WRITE_TOKEN;
        uint8_t tCh = 0;
        uint8_t tLen = 0;
        uint16_t tOffset = 0;

        uint16_t lcode = 0;
        uint16_t dcode = 0;
        uint8_t lextra = 0;
        uint8_t dextra = 0;

        outValue.strobe = 1;
        done = false;
        bool flag_out = false;
        // read ahead
        auto nextEncodedValue = inStream.read();

    huffman_loop:
        while (true) { // iterate over data in a block
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
            if (next_state == WRITE_TOKEN) {
                auto curEncodedValue = nextEncodedValue.data[0];
                // exit if already done
                if (done) break;
                // read next value if not done
                nextEncodedValue = inStream.read();
                if (nextEncodedValue.strobe == 0) {
                    done = true;
                }

                tCh = curEncodedValue.range(7, 0);
                tLen = curEncodedValue.range(15, 8);
                tOffset = curEncodedValue.range(31, 16);
                dcode = d_code(tOffset, dist_code);
                lcode = length_code[tLen];

                if (tLen > 0) {
                    next_state = ML_DIST_REP;
                } else {
                    outValue.data[0].code = litlnTree[tCh].code;
                    outValue.data[0].bitlen = litlnTree[tCh].bitlen;
                    hufCodeOutStream << outValue;
                    next_state = WRITE_TOKEN;
                }
            } else if (next_state == ML_DIST_REP) {
                outValue.data[0].code = litlnTree[lcode + c_literals + 1].code;
                outValue.data[0].bitlen = litlnTree[lcode + c_literals + 1].bitlen;
                lextra = extra_lbits[lcode];
                hufCodeOutStream << outValue;

                if (lextra)
                    next_state = ML_EXTRA;
                else
                    next_state = DIST_REP;

            } else if (next_state == DIST_REP) {
                outValue.data[0].code = distTree[dcode].code;
                outValue.data[0].bitlen = distTree[dcode].bitlen;
                dextra = extra_dbits[dcode];
                hufCodeOutStream << outValue;

                if (dextra)
                    next_state = DIST_EXTRA;
                else
                    next_state = WRITE_TOKEN;

            } else if (next_state == ML_EXTRA) {
                tLen -= (base_length[lcode] + 3);
                outValue.data[0].code = tLen;
                outValue.data[0].bitlen = lextra;
                hufCodeOutStream << outValue;

                next_state = DIST_REP;

            } else if (next_state == DIST_EXTRA) {
                tOffset -= base_dist[dcode];
                outValue.data[0].code = tOffset;
                outValue.data[0].bitlen = dextra;
                hufCodeOutStream << outValue;

                next_state = WRITE_TOKEN;
            }
        }
        // End of block as per GZip standard
        outValue.data[0].code = litlnTree[256].code;
        outValue.data[0].bitlen = litlnTree[256].bitlen;
        hufCodeOutStream << outValue;
        // indicate end of data in stream for this block
        outValue.strobe = 0;
        hufCodeOutStream << outValue;
    }
    // end of data
    outValue.strobe = 0;
    hufCodeOutStream << outValue;
}

// Content of called function
void huffmanProcessingUnit(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                           hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    enum HuffEncoderStates { READ_LITERAL, READ_MATCH_LEN, READ_OFFSET0, READ_OFFSET1 };
    IntVectorStream_dt<32, 1> outValue;
    outValue.strobe = 0;
    bool done = false;
    int blk_n = 0;
    while (true) { // iterate over multiple blocks in a file
        enum HuffEncoderStates next_state = READ_LITERAL;

        IntVectorStream_dt<9, 1> nextValue = inStream.read();
        // end of file
        if (nextValue.strobe == 0) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }
        outValue.data[0] = 0;
        outValue.strobe = 1;

        done = false;
    hf_proc_static_main:
        while (!done) { // iterate over data in block
#pragma HLS PIPELINE II = 1
            bool outFlag = false;
            ap_uint<9> inValue = nextValue.data[0];
            bool tokenFlag = (inValue.range(8, 8) == 1);
            nextValue = inStream.read();

            if (next_state == READ_LITERAL) {
                if (tokenFlag) {
                    outValue.data[0].range(7, 0) = 0;
                    outValue.data[0].range(15, 8) = inValue.range(7, 0);
                    outFlag = false;
                    next_state = READ_OFFSET0;
                } else {
                    outValue.data[0].range(7, 0) = inValue;
                    outFlag = true;
                    next_state = READ_LITERAL;
                }
            } else if (next_state == READ_OFFSET0) {
                outFlag = false;
                outValue.data[0].range(23, 16) = inValue;
                outValue.strobe++;
                next_state = READ_OFFSET1;
            } else if (next_state == READ_OFFSET1) {
                outFlag = true;
                outValue.data[0].range(31, 24) = inValue;
                outValue.strobe++;
                next_state = READ_LITERAL;
            }

            if (outFlag) {
                outStream << outValue;
                outValue.data[0] = 0;
                outFlag = false;
            }
            // exit condition
            if (nextValue.strobe == 0) {
                done = true;
            }
        }
        // end of block
        outValue.strobe = 0;
        outStream << outValue;
    }
}

// Content of called function
void bitPackingStream(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 2> >& outStream) {
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> inValue;
    IntVectorStream_dt<8, 2> outValue;
    int blk_n = 0;
    while (true) { // iterate over blocks
        ap_uint<32> localBits = 0;
        uint8_t localBits_idx = 0;
        bool last_block = true;

    bitpack:
        for (inValue = inStream.read(); inValue.strobe != 0; inValue = inStream.read()) {
#pragma HLS PIPELINE II = 1
            uint16_t val = inValue.data[0].code;
            uint8_t size = inValue.data[0].bitlen;
            localBits.range(size + localBits_idx - 1, localBits_idx) = val;
            localBits_idx += size;

            if (localBits_idx >= 16) {
                uint16_t pack_byte = localBits;
                localBits >>= 16;
                localBits_idx -= 16;

                outValue.strobe = 2;
                outValue.data[0] = (uint8_t)pack_byte;
                outValue.data[1] = (uint8_t)(pack_byte >> 8);
                outStream << outValue;
            }
            last_block = false;
        }

        // end of file
        if (last_block) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }

        int leftBits = localBits_idx % 8;
        int val = 0;
        ap_uint<64> packedBits = 0;

        if (leftBits) {
            // 3 bit header
            localBits_idx += 3;

            leftBits = localBits_idx % 8;

            if (leftBits != 0) {
                val = 8 - leftBits;
                localBits_idx += val;
            }

            // Zero bytes and complement as per type0 z_sync_flush
            uint64_t val = 0xffff0000;
            packedBits = localBits | (val << localBits_idx);
            localBits_idx += 32;

        } else {
            // Zero bytes and complement as per type0 z_sync_flush
            uint64_t val = 0xffff000000;
            packedBits = localBits | (val << localBits_idx);
            localBits_idx += 40;
        }
        for (uint32_t i = 0; i < localBits_idx; i += 16) {
#pragma HLS PIPELINE II = 1
            uint16_t pack_byte = packedBits;
            packedBits >>= 16;

            outValue.data[0] = pack_byte;
            if (localBits_idx - i < 9) {
                outValue.strobe = 1;
            } else {
                outValue.strobe = 2;
                outValue.data[1] = (uint8_t)(pack_byte >> 8);
            }
            outStream << outValue;
        }
        // end of block
        outValue.strobe = 0;
        outValue.data[0] = 0;
        outStream << outValue;
    }
}

// Content of called function
void receiveBuffer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                   hls::stream<IntVectorStream_dt<IN_DATAWIDTH, 1> >& outStream,
                   hls::stream<ap_uint<17> >& inputSize) {
    IntVectorStream_dt<IN_DATAWIDTH, 1> outVal;

buffer_top:
    while (1) {
        ap_uint<17> inSize = inputSize.read();
        ap_uint<IN_DATAWIDTH> inVal = 0;
        // proceed further if valid size
        if (inSize == 0) break;
        auto outSizeV = inSize;
        outVal.strobe = 1;
    buffer_assign:
        for (auto i = 0; i < outSizeV; i++) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            outVal.data[0] = inVal;
            outStream << outVal;
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}

// Content of called function
void simpleStreamUpsizer(hls::stream<IntVectorStream_dt<8, IN_WIDTH / 8> >& inStream,
                         hls::stream<ap_uint<OUT_WIDTH + SIZE_DWIDTH> >& outStream) {
    constexpr uint8_t c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr uint8_t c_inBytes = IN_WIDTH / 8;
    ap_uint<IN_WIDTH> inVal;
    ap_uint<OUT_WIDTH> outVal;
    bool last = false;
    ap_uint<4> dsize;

stream_upsizer_top:
    while (!last) {
        last = true;
        uint8_t byteIdx = 0;
        dsize = 0;
        auto inStVal = inStream.read();
        bool loop_continue = (inStVal.strobe != 0);
    stream_upsizer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            if (byteIdx == c_upsizeFactor) {
                ap_uint<SIZE_DWIDTH + OUT_WIDTH> tmpVal = outVal;
                tmpVal <<= SIZE_DWIDTH;
                tmpVal.range(SIZE_DWIDTH - 1, 0) = dsize;
                outStream << tmpVal;
                byteIdx = 0;
                dsize = 0;
                loop_continue = (inStVal.strobe != 0);
            }
        upszr_assign_input:
            for (uint8_t b = 0; b < c_inBytes; ++b) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 0 max = c_inBytes
                if (b < inStVal.strobe) inVal.range(((b + 1) * 8) - 1, b * 8) = inStVal.data[b];
            }
            outVal >>= IN_WIDTH;
            outVal.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = inVal;
            ++byteIdx;
            dsize += inStVal.strobe;
            if (inStVal.strobe != 0) inStVal = inStream.read();
        }
        // end of block/files
        outStream << 0;
    }
}

// Content of called function
void sendBuffer(hls::stream<IntVectorStream_dt<D_WIDTH, 1> >& inStream,
                hls::stream<ap_uint<D_WIDTH> >& outStream,
                hls::stream<ap_uint<17> >& outSize) {
    bool last = false;

buffer_top:
    while (!last) {
        last = true;
        ap_uint<17> sizeCntr = 0;
        auto inVal = inStream.read();
        bool loop_continue = (inVal.strobe != 0);
    buffer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            loop_continue = (inVal.strobe != 0);
            if (!loop_continue) break;
            outStream << inVal.data[0];
            if (inVal.strobe != 0) {
                inVal = inStream.read();
                sizeCntr++;
            }
        }
        // write out size of up-sized data to terminate the block
        outSize << sizeCntr;
    }
}