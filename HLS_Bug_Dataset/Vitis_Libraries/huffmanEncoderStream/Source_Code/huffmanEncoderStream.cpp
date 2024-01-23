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