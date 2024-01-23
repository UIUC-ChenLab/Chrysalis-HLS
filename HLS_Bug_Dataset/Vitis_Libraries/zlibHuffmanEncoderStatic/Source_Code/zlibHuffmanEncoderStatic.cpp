void zlibHuffmanEncoderStatic(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                              hls::stream<IntVectorStream_dt<8, 2> >& huffOut) {
#pragma HLS DATAFLOW
    hls::stream<IntVectorStream_dt<32, 1> > encodedOutStream("encodedOutStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream("hufCodeStream");
#pragma HLS STREAM variable = encodedOutStream depth = 4
#pragma HLS STREAM variable = hufCodeStream depth = 4

    xf::compression::details::huffmanProcessingUnit(inStream, encodedOutStream);
    xf::compression::huffmanEncoderStatic(encodedOutStream, hufCodeStream);
    xf::compression::details::bitPackingStatic(hufCodeStream, huffOut);
}

// Content of called function
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

// Content of called function
void bitPackingStatic(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 2> >& outStream) {
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> inValue;
    IntVectorStream_dt<8, 2> outValue;

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
            uint16_t pack_byte = packedBits;
            packedBits >>= 16;

            outValue.data[0] = pack_byte;
            if (localBits_idx - i < 16) {
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