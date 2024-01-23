void huffmanEncoderStatic(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& outStream) {
    uint8_t extra_lbits[c_length_codes] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                           2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    uint8_t extra_dbits[c_distance_codes] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                             6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    enum huffmanEncoderState { WRITE_TOKEN, ML_DIST_REP, LIT_REP, SEND_OUTPUT, ML_EXTRA, DIST_REP, DIST_EXTRA, DONE };
    bool done = false;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outValue;

    while (true) { // iterate over blocks
        enum huffmanEncoderState next_state = WRITE_TOKEN;
        uint8_t tCh = 0;
        uint8_t tLen = 0;
        uint16_t tOffset = 0;

        uint16_t lcode = 0;
        uint16_t dcode = 0;
        uint8_t lextra = 0;
        uint8_t dextra = 0;

        auto nextEncodedValue = inStream.read();
        if (nextEncodedValue.strobe == 0) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }

        outValue.strobe = 1;

        // ZLIB Header for Static huffmanTree
        outValue.data[0].code = 2;
        outValue.data[0].bitlen = 3;
        outStream << outValue;
        done = false;
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
                    outValue.data[0].code = lit_code_fixed[tCh];
                    outValue.data[0].bitlen = lit_blen_fixed[tCh];
                    outStream << outValue;

                    next_state = WRITE_TOKEN;
                }
            } else if (next_state == ML_DIST_REP) {
                uint16_t code_s = lit_code_fixed[lcode + c_literals + 1];
                uint8_t bitlen = lit_blen_fixed[lcode + c_literals + 1];
                lextra = extra_lbits[lcode];

                outValue.data[0].code = code_s;
                outValue.data[0].bitlen = bitlen;
                outStream << outValue;

                if (lextra)
                    next_state = ML_EXTRA;
                else
                    next_state = DIST_REP;

            } else if (next_state == DIST_REP) {
                uint16_t code_s = dist_codes_fixed[dcode];
                uint8_t bitlen = dist_blen_fixed[dcode];
                dextra = extra_dbits[dcode];

                outValue.data[0].code = code_s;
                outValue.data[0].bitlen = bitlen;
                outStream << outValue;

                if (dextra)
                    next_state = DIST_EXTRA;
                else
                    next_state = WRITE_TOKEN;

            } else if (next_state == ML_EXTRA) {
                tLen -= (base_length[lcode] + 3);
                outValue.data[0].code = tLen;
                outValue.data[0].bitlen = lextra;
                outStream << outValue;

                next_state = DIST_REP;

            } else if (next_state == DIST_EXTRA) {
                tOffset -= base_dist[dcode];
                outValue.data[0].code = tOffset;
                outValue.data[0].bitlen = dextra;
                outStream << outValue;

                next_state = WRITE_TOKEN;
            }
        }
        // End of block as per GZip standard
        outValue.data[0].code = lit_code_fixed[256];
        outValue.data[0].bitlen = lit_blen_fixed[256];
        outStream << outValue;
        // indicate end of data in stream for this block
        outValue.strobe = 0;
        outStream << outValue;
    }
}