void lzDecompressZlibEos(hls::stream<ap_uint<32> >& inStream,
                         hls::stream<bool>& inStream_eos,
                         hls::stream<ap_uint<8> >& outStream,
                         hls::stream<bool>& outStream_eos,
                         hls::stream<uint64_t>& outSize_val) {
    enum lz_d_states { READ_STATE, MATCH_STATE, LOW_OFFSET_STATE };
    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false

    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    lz_d_states next_states = READ_STATE;
    uint16_t offset = 0;
    ap_uint<32> nextValue, currValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY_PARTITION variable = prevValue dim = 0 complete
    uint64_t out_cntr = 0;

    bool eos_flag = inStream_eos.read();
    nextValue = inStream.read();
lz_decompress:
    for (uint32_t i = 0; (eos_flag == false) || (next_states != READ_STATE); i++) {
#pragma HLS PIPELINE II = 1
        ////printme("Start of the loop %d state %d \n", i, next_states);
        if (next_states == READ_STATE) {
            currValue = nextValue;
            eos_flag = inStream_eos.read();
            nextValue = inStream.read();
            offset = currValue.range(15, 0);
            length_extract = currValue.range(31, 16);
            // printme("offset %d length_extract %d \n", offset, length_extract);
            if (length_extract) {
                match_loc = i - offset;
                match_len = length_extract;
                ////printme("HISTORY=%x\n",(uint8_t)outValue);
                out_len = match_len - 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset - 1];
                }
                match_loc++;
            } else {
                outValue = currValue.range(7, 0);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset - 1];
            match_loc++;
            if (out_len == 1) {
                next_states = READ_STATE;
            }
            if (out_len) {
                out_len--;
            }
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            ////printme("HISTORY=%x\n",(uint8_t)outValue);
            match_loc++;
            if (out_len == 1) {
                next_states = READ_STATE;
            }
            if (out_len) {
                out_len--;
            }
        }
        local_buf[i % HISTORY_SIZE] = outValue;

        outStream << outValue;
        out_cntr++;
        outStream_eos << 0;

        // printme("%c", (uint8_t)outValue);
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }
    // printme("Exited main for-llop \n");

    outStream << 0;
    outStream_eos << 1;

    outSize_val << out_cntr;
}