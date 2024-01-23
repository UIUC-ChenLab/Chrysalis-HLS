void lzCompress(hls::stream<ap_uint<8> >& inStream, hls::stream<ap_uint<32> >& outStream, uint32_t input_size) {
    const int c_dictEleWidth = (MATCH_LEN * 8 + 24);
    typedef ap_uint<MATCH_LEVEL * c_dictEleWidth> uintDictV_t;
    typedef ap_uint<c_dictEleWidth> uintDict_t;

    if (input_size == 0) return;
    // Dictionary
    uintDictV_t dict[LZ_DICT_SIZE];
#pragma HLS BIND_STORAGE variable = dict type = RAM_T2P impl = URAM
    uintDictV_t resetValue = 0;
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

    uint8_t present_window[MATCH_LEN];
#pragma HLS ARRAY_PARTITION variable = present_window complete
    for (uint8_t i = 1; i < MATCH_LEN; i++) {
#pragma HLS PIPELINE off
        present_window[i] = inStream.read();
    }
lz_compress:
    for (uint32_t i = MATCH_LEN - 1; i < input_size - LEFT_BYTES; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = dict inter false
        uint32_t currIdx = i - MATCH_LEN + 1;
        // shift present window and load next value
        for (int m = 0; m < MATCH_LEN - 1; m++) {
#pragma HLS UNROLL
            present_window[m] = present_window[m + 1];
        }
        present_window[MATCH_LEN - 1] = inStream.read();

        // Calculate Hash Value
        uint32_t hash = 0;
        if (MIN_MATCH == 3) {
            hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^
                   (present_window[0] << 1) ^ (present_window[1]);
        } else {
            hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^ (present_window[3]);
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
            for (int m = 0; m < MATCH_LEN; m++) {
                if (present_window[m] == compareWith.range((m + 1) * 8 - 1, m * 8) && !done) {
                    len++;
                } else {
                    done = 1;
                }
            }
            if ((len >= MIN_MATCH) && (currIdx > compareIdx) && ((currIdx - compareIdx) < LZ_MAX_OFFSET_LIMIT) &&
                ((currIdx - compareIdx - 1) >= MIN_OFFSET)) {
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
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = present_window[0];
        outValue.range(15, 8) = match_length;
        outValue.range(31, 16) = match_offset;
        outStream << outValue;
    }
lz_compress_leftover:
    for (int m = 1; m < MATCH_LEN; m++) {
#pragma HLS PIPELINE
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = present_window[m];
        outStream << outValue;
    }
lz_left_bytes:
    for (int l = 0; l < LEFT_BYTES; l++) {
#pragma HLS PIPELINE
        ap_uint<32> outValue = 0;
        outValue.range(7, 0) = inStream.read();
        outStream << outValue;
    }
}