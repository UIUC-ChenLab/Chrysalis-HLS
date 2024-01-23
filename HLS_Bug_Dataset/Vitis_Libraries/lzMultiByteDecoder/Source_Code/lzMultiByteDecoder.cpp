void lzMultiByteDecoder(hls::stream<SIZE_DT>& litlenStream,
                        hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream,
                        hls::stream<ap_uint<16> >& offsetStream,
                        hls::stream<SIZE_DT>& matchlenStream,
                        hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                        hls::stream<ap_uint<PARALLEL_BYTES> >& outStrb) {
    const uint8_t c_parallelBit = PARALLEL_BYTES * 8;
    const uint8_t c_lowOffset = 4 * PARALLEL_BYTES;
    const uint8_t c_veryLowOffset = 2 * PARALLEL_BYTES;

    const uint16_t c_ramHistSize = HISTORY_SIZE / PARALLEL_BYTES;
    const uint8_t c_regHistSize = (2 * c_lowOffset) / PARALLEL_BYTES;

    enum lzDecompressStates { WRITE_LITERAL, READ_MATCH, NO_OP };
    enum lzDecompressStates next_state = WRITE_LITERAL; // start from Read Literal Length

    ap_uint<c_parallelBit> ramHistory[2][c_ramHistSize];
#pragma HLS dependence variable = ramHistory inter false
#pragma HLS BIND_STORAGE variable = ramHistory type = RAM_2P impl = URAM
#pragma HLS ARRAY_PARTITION variable = ramHistory dim = 1 complete

    ap_uint<c_parallelBit> regHistory[2][c_regHistSize];
// full partition  to infer as reg
#pragma HLS ARRAY_PARTITION variable = regHistory dim = 0 complete

    SIZE_DT lit_len = 0;
    SIZE_DT orig_lit_len = 0;
    uint32_t output_cnt = 0;
    uint16_t match_loc = 0;
    SIZE_DT match_len = 0;
    uint16_t write_idx = 0;
    uint16_t output_index = 0;
    uint32_t outSize = 0;

    bool outStreamFlag = false;

    ap_uint<16> offset = 0;
    ap_uint<c_parallelBit> outStreamValue = 0;
    ap_uint<2 * c_parallelBit> output_window;
    uint8_t parallelBits = 0;

    ap_uint<c_parallelBit> outValue = 0;
    bool matchDone = false;
    orig_lit_len = litlenStream.read();
    lit_len = orig_lit_len;
    output_cnt += lit_len;

    if (orig_lit_len == 0) {
        matchDone = true;
        match_len = matchlenStream.read();
        offset = offsetStream.read();
    }

    uint16_t read_idx = match_loc / PARALLEL_BYTES;
    uint16_t byte_loc = (match_loc % PARALLEL_BYTES) % PARALLEL_BYTES;

    bool outFlag = false;
    uint8_t incr_output_index = 0;
lz4_decoder:
    for (; matchDone == false;) {
#pragma HLS PIPELINE II = 1
        bool veryLowOffsetFlag = false, matchLocFlag = false;
        ap_uint<2 * c_parallelBit> localValue;
        ap_uint<c_parallelBit> lowValue, highValue;

        // always reading to make better timing
        ap_uint<c_parallelBit> lowValueReg = regHistory[0][(read_idx + 0) % c_regHistSize];
        ap_uint<c_parallelBit> highValueReg = regHistory[1][(read_idx + 1) % c_regHistSize];
        ap_uint<c_parallelBit> lowValueRam = ramHistory[0][(read_idx + 0) % c_ramHistSize];
        ap_uint<c_parallelBit> highValueRam = ramHistory[1][(read_idx + 1) % c_ramHistSize];

        if (offset < c_lowOffset) {
            lowValue = lowValueReg;
            highValue = highValueReg;
        } else {
            lowValue = lowValueRam;
            highValue = highValueRam;
        }

        localValue.range(c_parallelBit - 1, 0) = lowValue;
        localValue.range(2 * c_parallelBit - 1, c_parallelBit) = highValue;
        ap_uint<c_parallelBit> matchValue = localValue >> (byte_loc * 8);

        if (next_state == WRITE_LITERAL) {
            outFlag = true;
            // printf("WRITE_LITERAL\n");
            outValue = litStream.read();
            if (lit_len <= PARALLEL_BYTES) {
                incr_output_index = lit_len;
                lit_len = 0;
                offset = offsetStream.read();
                match_len = matchlenStream.read();
                match_loc = output_cnt - offset;
                output_cnt += match_len;
                if ((offset > 0) & (offset < c_veryLowOffset)) {
                    parallelBits = 1;
                    if (offset < (2 * PARALLEL_BYTES)) {
                        next_state = NO_OP;
                    } else {
                        next_state = READ_MATCH;
                    }
                } else {
                    parallelBits = PARALLEL_BYTES;
                    next_state = READ_MATCH;
                }
            } else {
                incr_output_index = PARALLEL_BYTES;
                lit_len -= PARALLEL_BYTES;
                next_state = WRITE_LITERAL;
            }
        } else if (next_state == READ_MATCH) {
            // printf("READ_MATCH\n");
            outFlag = true;
            outValue = matchValue;

            if (match_len <= parallelBits) {
                incr_output_index = match_len;
                match_loc += match_len;
                match_len = 0;
                orig_lit_len = litlenStream.read();
                lit_len = orig_lit_len;
                output_cnt += orig_lit_len;
                if (orig_lit_len) {
                    next_state = WRITE_LITERAL;
                } else {
                    offset = offsetStream.read();
                    match_len = matchlenStream.read();
                    match_loc = output_cnt - offset;
                    if ((offset > 0) & (offset < c_veryLowOffset)) {
                        parallelBits = 1;
                        if (offset < (2 * PARALLEL_BYTES)) {
                            next_state = NO_OP;
                        } else {
                            next_state = READ_MATCH;
                        }
                    } else {
                        parallelBits = PARALLEL_BYTES;
                        next_state = READ_MATCH;
                    }
                }
                output_cnt += match_len;
            } else {
                incr_output_index = parallelBits;
                match_loc += parallelBits;
                match_len -= parallelBits;
                next_state = READ_MATCH;
            }
        } else if (next_state == NO_OP) {
            outFlag = false;
            // printf("NO_OP\n");
            // Adding NO_OP as workaround for low offset case as
            // for very low offset case, results are not matching
            next_state = READ_MATCH;
        } else {
            assert(0);
        }

        if (orig_lit_len == 0 && match_len == 0) {
            matchDone = true;
        }

        read_idx = match_loc / PARALLEL_BYTES;
        byte_loc = (match_loc % PARALLEL_BYTES) % PARALLEL_BYTES;

        if (outFlag) {
            output_window.range((output_index + PARALLEL_BYTES) * 8 - 1, output_index * 8) = outValue;
            output_index += incr_output_index;
        }
        uint8_t localOutputIdx = output_index - PARALLEL_BYTES;
        bool outputIdxFlag = ((output_index >= PARALLEL_BYTES));

        outStreamValue = output_window.range(c_parallelBit - 1, 0);
        regHistory[0][write_idx % c_regHistSize] = outStreamValue;
        regHistory[1][write_idx % c_regHistSize] = outStreamValue;
        ramHistory[0][write_idx % c_ramHistSize] = outStreamValue;
        ramHistory[1][write_idx % c_ramHistSize] = outStreamValue;

        bool outStreamFlag = false;
        if (outputIdxFlag) {
            write_idx++;
            output_window >>= PARALLEL_BYTES * 8;
            output_index = localOutputIdx;
            outStreamFlag = true;
        }

        if (outStreamFlag) {
            outStream << outStreamValue;
            outStrb << -1;
            outSize += PARALLEL_BYTES;
        }
    }

    // output_index:%d\n",lit_len,match_len,incr_output_index,output_index);
    // Write out if there is remaining left over data in output buffer
    // to outStream
    if (output_index) {
        outStreamValue = output_window.range(c_parallelBit - 1, 0);
        outStream << outStreamValue;
        outStrb << ((1 << output_index) - 1);
        outSize += output_index;
    }
    outStream << 0;
    outStrb << 0;
}

// Content of called function
T reg(T d) {
#pragma HLS PIPELINE II = 1
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INLINE off
    return d;
}