void lzDecompress(hls::stream<ap_uint<32> >& inStream, hls::stream<ap_uint<8> >& outStream, uint32_t original_size) {
    enum lzDecompressStates { READ_STATE, MATCH_STATE, LOW_OFFSET_STATE };

    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false

    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    enum lzDecompressStates next_states = READ_STATE;
    uint16_t offset = 0;
    ap_uint<32> nextValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY_PARTITION variable = prevValue dim = 0 complete
lz_decompress:
    for (uint32_t i = 0; i < original_size; i++) {
#pragma HLS PIPELINE II = 1
        if (next_states == READ_STATE) {
            nextValue = inStream.read();
            offset = nextValue.range(15, 0);
            length_extract = nextValue.range(31, 16);
            if (length_extract) {
                match_loc = i - offset - 1;
                match_len = length_extract + 1;
                out_len = 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset];
                }
                match_loc++;
            } else {
                outValue = nextValue.range(7, 0);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset];
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        }
        local_buf[i % HISTORY_SIZE] = outValue;
        outStream << outValue;
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }
}