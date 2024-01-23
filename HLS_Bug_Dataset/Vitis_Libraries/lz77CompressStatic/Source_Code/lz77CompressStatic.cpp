void lz77CompressStatic(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                        hls::stream<IntVectorStream_dt<9, 1> >& lz77Out) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream1("compressedStream1");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
#pragma HLS STREAM variable = compressedStream depth = 4
#pragma HLS STREAM variable = compressedStream1 depth = 4
#pragma HLS STREAM variable = boosterStream depth = 4

    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_ID>(
        inStream, compressedStream);
    xf::compression::lzBestMatchFilter<MATCH_LEN, LZ_MAX_OFFSET_LIMIT>(compressedStream, compressedStream1);
    xf::compression::lzBooster<MAX_MATCH_LEN, MAX_BLOCK_SIZE>(compressedStream1, boosterStream);
    xf::compression::lz77DivideStatic(boosterStream, lz77Out);
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