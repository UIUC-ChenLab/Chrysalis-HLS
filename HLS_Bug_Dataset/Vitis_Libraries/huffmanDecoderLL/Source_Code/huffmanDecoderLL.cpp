void huffmanDecoderLL(hls::stream<ap_uint<16> >& inStream,
                      hls::stream<bool>& inEos,
                      hls::stream<ap_uint<16> >& outStream
#ifdef GZIP_DECOMPRESS_CHECKSUM
                      ,
                      hls::stream<ap_uint<32> >& checkSum
#endif
                      ) {
    bitBufferTypeLL bitbuffer = 0;
    ap_uint<6> bits_cntr = 0;
    bool isMultipleFiles = false;
    bool done = false;
huffmanDecoder_label0:
    while (done == false) {
        uint8_t current_op = 0;
        uint8_t current_bits = 0;
        uint16_t current_val = 0;
        ap_uint<32> current_table_val;

        uint8_t len = 0;

        const ap_uint<5> order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

        bool dynamic_last = 0;
        ap_uint<9> dynamic_nlen = 0;
        ap_uint<9> dynamic_ndist = 0;
        ap_uint<5> dynamic_ncode = 0;
        ap_uint<9> dynamic_curInSize = 0;
        ap_uint<5> dynamic_lens[318];
#pragma HLS BIND_STORAGE variable = dynamic_lens type = ram_1p impl = lutram

        bool blocks_processed = false;

        bool isGzip = false;
        bool isZlib = false;
        // New huffman code
        ap_uint<16> codeOffsets[2][15];
#pragma HLS ARRAY_PARTITION variable = codeOffsets dim = 1 complete
        ap_uint<9> bl1Code[2][2];
        ap_uint<9> bl2Code[2][4];
        ap_uint<9> bl3Code[2][8];
        ap_uint<9> bl4Code[2][16];
        ap_uint<9> bl5Code[2][32];
        ap_uint<9> bl6Code[2][64];
        ap_uint<9> bl7Code[2][128];
        ap_uint<9> bl8Code[2][256];
        ap_uint<9> bl9Code[2][256];
        ap_uint<9> bl10Code[2][256];
        ap_uint<9> bl11Code[2][256];
        ap_uint<9> bl12Code[2][256];
        ap_uint<9> bl13Code[2][256];
        ap_uint<9> bl14Code[2][256];
        ap_uint<9> bl15Code[2][256];
#pragma HLS BIND_STORAGE variable = bl1Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl1Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl2Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl2Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl3Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl3Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl4Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl4Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl5Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl5Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl6Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl6Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl7Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl7Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl8Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl8Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl9Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl9Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl10Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl10Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl11Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl11Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl12Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl12Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl13Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl13Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl14Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl14Code complete dim = 1
#pragma HLS BIND_STORAGE variable = bl15Code type = ram_1p impl = lutram
#pragma HLS ARRAY_PARTITION variable = bl15Code complete dim = 1
        const bool include_fixed_block = (DECODER == FIXED || DECODER == FULL);
        const bool include_dynamic_block = (DECODER == DYNAMIC || DECODER == FULL);
        bool skip_fname = false;
        details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
        uint16_t magic_number = bitbuffer & 0xFFFF;
        details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
        if (magic_number == 0x8b1f && (FORMAT == BOTH)) {
            // GZIP Header Processing
            // Deflate mode & file name flag
            isGzip = true;
            isMultipleFiles = false;
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            uint16_t lcl_tmp = bitbuffer & 0xFFFF;
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);

            // Check for fnam content
            skip_fname = (lcl_tmp >> 8) ? true : false;
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);

            // MTIME - must
            // XFL (2 for high compress, 4 fast)
            // OS code (3Unix, 0Fat)
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);

            // MTIME - must
            // XFL (2 for high compress, 4 fast)
            // OS code (3Unix, 0Fat)
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
            // If FLG is set to zero by using -n
            if (skip_fname) {
                // Read file name
                details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                lcl_tmp = bitbuffer & 0xFF;
                details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)8);
            read_fname:
                while (lcl_tmp != 0) {
#pragma HLS PIPELINE II = 1
                    lcl_tmp = bitbuffer & 0xFF;

                    bits_cntr -= 8;

                    if (bits_cntr < 16 && (done == false)) {
                        uint16_t tmp_data = inStream.read();
                        bitbuffer = (bitbuffer >> 8) | (bitBufferTypeLL)(tmp_data) << bits_cntr;
                        done = inEos.read();
                        bits_cntr += 16;
                    } else {
                        bitbuffer >>= 8;
                    }
                }
            }
        } else if ((magic_number & 0x00FF) != 0x0078) {
            blocks_processed = true;
            isZlib = true;
        }

        if (isMultipleFiles) blocks_processed = true;
        while (!blocks_processed && (done == false)) {
            // one block per iteration
            // check if the following block is stored block or compressed block
            isMultipleFiles = true;
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            // read the last bit in bitbuffer to check if this is last block
            dynamic_last = bitbuffer & 1;
            bitbuffer >>= 1; // dump the bit read
            ap_uint<2> cb_type = (uint8_t)(bitbuffer)&3;
            bitbuffer >>= 2;
            bits_cntr -= 3; // previously dumped 1 bit + current dumped 2 bits

            if (cb_type == 0) { // stored block
                bitbuffer >>= bits_cntr & 7;
                bits_cntr -= bits_cntr & 7;

                details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                uint16_t store_length = bitbuffer & 0xffff;
                details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
                details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);

                if (DECODER == FULL) {
                    ap_uint<16> tmpVal = 0;
                    details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                strd_blk_cpy:
                    for (uint16_t i = 0; i < store_length; i++) {
#pragma HLS PIPELINE II = 1
                        tmpVal.range(7, 0) = bitbuffer;
                        tmpVal.range(15, 8) = 0xF0;
                        outStream << tmpVal;

                        bits_cntr -= 8;
                        if (bits_cntr < 16 && (done == false)) {
                            uint16_t tmp_data = inStream.read();
                            bitbuffer = (bitbuffer >> 8) | (bitBufferTypeLL)(tmp_data) << bits_cntr;
                            done = inEos.read();
                            bits_cntr += 16;
                        } else {
                            bitbuffer >>= 8;
                        }
                    }
                }

            } else if (cb_type == 2) {       // dynamic huffman compressed block
                if (include_dynamic_block) { // compile if decoder should be dynamic/full
                                             // Read 14 bits HLIT(5-bits), HDIST(5-bits) and HCLEN(4-bits)
                    details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                    dynamic_nlen = (bitbuffer & ((1 << 5) - 1)) + 257; // Max 288
                    details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)5);

                    dynamic_ndist = (bitbuffer & ((1 << 5) - 1)) + 1; // Max 30
                    details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)5);

                    dynamic_ncode = (bitbuffer & ((1 << 4) - 1)) + 4; // Max 19
                    details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)4);

                    dynamic_curInSize = 0;

                dyn_len_bits:
                    while (dynamic_curInSize < dynamic_ncode) {
#pragma HLS PIPELINE II = 2
                        if ((bits_cntr < 16) && (done == false)) {
                            uint16_t tmp_data = inStream.read();
                            bitbuffer += (bitBufferTypeLL)(tmp_data << bits_cntr);
                            done = inEos.read();
                            bits_cntr += 16;
                        }
                        dynamic_lens[order[dynamic_curInSize++]] = bitbuffer & ((1 << 3) - 1);
                        details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)3);
                    }

                    while (dynamic_curInSize < 19) dynamic_lens[order[dynamic_curInSize++]] = 0;
                    details::code_generator_array_dyn_new(1, dynamic_lens, 19, codeOffsets[0], bl1Code[0], bl2Code[0],
                                                          bl3Code[0], bl4Code[0], bl5Code[0], bl6Code[0], bl7Code[0],
                                                          bl8Code[0], bl9Code[0], bl10Code[0], bl11Code[0], bl12Code[0],
                                                          bl13Code[0], bl14Code[0], bl15Code[0]);

                    details::byteGen(bitbuffer, bits_cntr, codeOffsets[0], bl1Code[0], bl2Code[0], bl3Code[0],
                                     bl4Code[0], bl5Code[0], bl6Code[0], bl7Code[0], dynamic_lens, inEos, inStream,
                                     dynamic_nlen, dynamic_ndist, done);

                    details::code_generator_array_dyn_new(2, dynamic_lens, dynamic_nlen, codeOffsets[0], bl1Code[0],
                                                          bl2Code[0], bl3Code[0], bl4Code[0], bl5Code[0], bl6Code[0],
                                                          bl7Code[0], bl8Code[0], bl9Code[0], bl10Code[0], bl11Code[0],
                                                          bl12Code[0], bl13Code[0], bl14Code[0], bl15Code[0]);

                    details::code_generator_array_dyn_new(
                        3, dynamic_lens + dynamic_nlen, dynamic_ndist, codeOffsets[1], bl1Code[1], bl2Code[1],
                        bl3Code[1], bl4Code[1], bl5Code[1], bl6Code[1], bl7Code[1], bl8Code[1], bl9Code[1], bl10Code[1],
                        bl11Code[1], bl12Code[1], bl13Code[1], bl14Code[1], bl15Code[1]);
                    // BYTEGEN dynamic state
                    // ********************************
                    //  Create Packets Below
                    //  [LIT|ML|DIST|DIST] --> 32 Bit
                    //  Read data from inStream - 8bits
                    //  at a time. Decode the literals,
                    //  ML, Distances based on tables
                    // ********************************

                    // Read from inStream
                    details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
                    uint8_t ignoreValue = (dynamic_last) ? 0xFD : 0XFE;
                    uint8_t ret = details::huffmanBytegenLL(bitbuffer, bits_cntr, outStream, inEos, inStream,
                                                            codeOffsets, bl1Code, bl2Code, bl3Code, bl4Code, bl5Code,
                                                            bl6Code, bl7Code, bl8Code, bl9Code, bl10Code, bl11Code,
                                                            bl12Code, bl13Code, bl14Code, bl15Code, done, ignoreValue);

                } else {
                    blocks_processed = true;
                }
            } else if (cb_type == 1) {     // fixed huffman compressed block
                if (include_fixed_block) { // compile if decoder should be fixed/full
                    // ********************************

                    details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);

                    // BitCodes
                    ap_uint<15> bit7 = 0;
                    ap_uint<15> bit8 = 48;
                    ap_uint<15> bit9 = 400;
                    for (ap_uint<9> i = 0; i < 288; i++) {
#pragma HLS PIPELINE II = 1
                        // codeoffsets
                        if (i == 4) {
                            codeOffsets[0][i] = -1;
                            codeOffsets[1][i] = 0;
                        } else if (i == 6) {
                            codeOffsets[0][i] = 0;
                            codeOffsets[1][i] = -1;
                        } else if (i == 7) {
                            codeOffsets[0][i] = 48;
                            codeOffsets[1][i] = -1;
                        } else if (i == 8) {
                            codeOffsets[0][i] = 400;
                            codeOffsets[1][i] = -1;
                        } else if (i < 15) {
                            codeOffsets[0][i] = -1;
                            codeOffsets[1][i] = -1;
                        }

                        // bitcodes
                        if (i < 32) { // fill distance codes
                            bl5Code[1][i] = i;
                        }
                        if (i < 144) { // literal upto 144
                            bl8Code[0][bit8] = i;
                            bit8 += 1;
                        } else if (i < 256) { // literal upto 256
                            bl9Code[0][bit9.range(7, 0)] = i;
                            bit9 += 1;
                        } else if (i < 280) {
                            bl7Code[0][bit7] = i;
                            bit7 += 1;
                        } else {
                            bl8Code[0][bit8] = i;
                            bit8 += 1;
                        }
                    }
                    uint8_t ignoreValue = (dynamic_last) ? 0xFD : 0XFE;
                    uint8_t ret = details::huffmanBytegenLL(bitbuffer, bits_cntr, outStream, inEos, inStream,
                                                            codeOffsets, bl1Code, bl2Code, bl3Code, bl4Code, bl5Code,
                                                            bl6Code, bl7Code, bl8Code, bl9Code, bl10Code, bl11Code,
                                                            bl12Code, bl13Code, bl14Code, bl15Code, done, ignoreValue);
                } else {
                    blocks_processed = true;
                }
            } else {
                blocks_processed = true;
            }
            if (dynamic_last) blocks_processed = true;
        } // While end
        // Checksum 4Bytes
        ap_uint<6> leftOverBits = bits_cntr % 8;
        details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)leftOverBits);
        ap_uint<32> chckVar = 0;
        details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
        chckVar.range(23, 16) = bitbuffer.range(15, 8);
        chckVar.range(31, 24) = bitbuffer.range(7, 0);
        details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
        details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
        chckVar.range(7, 0) = bitbuffer.range(15, 8);
        chckVar.range(15, 8) = bitbuffer.range(7, 0);
        details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
#ifdef GZIP_DECOMPRESS_CHECKSUM
        checkSum << chckVar;
#endif
        // FileSize in case of Gzip
        if ((FORMAT == BOTH) && isGzip) {
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
            details::loadBitStreamLL(bitbuffer, bits_cntr, inStream, inEos, done);
            details::discardBitStreamLL(bitbuffer, bits_cntr, (ap_uint<6>)16);
        } else {
        consumeLeftOverData:
            while (done == false) {
                inStream.read();
                done = inEos.read();
            }
        }
    }
    outStream << 0xFFFF; // Adding Dummy Data for last end of stream case
}

// Content of called function
void byteGen(bitBufferTypeLL& _bitbuffer,
             ap_uint<6>& bits_cntr,
             ap_uint<16>* codeOffsets,
             ap_uint<9>* bl1Codes,
             ap_uint<9>* bl2Codes,
             ap_uint<9>* bl3Codes,
             ap_uint<9>* bl4Codes,
             ap_uint<9>* bl5Codes,
             ap_uint<9>* bl6Codes,
             ap_uint<9>* bl7Codes,
             ap_uint<5>* lens,
             hls::stream<bool>& inEos,
             hls::stream<ap_uint<16> >& inStream,
             ap_uint<9> nlen,
             ap_uint<9> ndist,
             bool& done) {
    loadBitStreamLL(_bitbuffer, bits_cntr, inStream, inEos, done);
    uint8_t copy = 0;
    uint8_t extra_copy = 0;
    uint8_t nNum = 0;
    ap_uint<7> validCodeOffset;
    ap_uint<5> symbol[8];
#pragma HLS ARRAY_PARTITION variable = symbol dim = 1 complete
    uint16_t len = 0;
    uint16_t dynamic_curInSize = 0;
    bitBufferTypeLL bitbuffer[2];
    bool isExtra = false;
    ap_uint<3> extra = 0;
    ap_uint<4> current_bits = 0;
bytegen:
    while ((dynamic_curInSize < nlen + ndist) || (copy != 0)) {
#pragma HLS PIPELINE II = 1
        for (ap_uint<5> i = 0; i < 7; i++) {
#pragma HLS UNROLL
            bool val = (_bitbuffer.range(0, i) >= codeOffsets[i]) ? 1 : 0;
            if (val) {
                current_bits = i + 1;
            }
        }
        symbol[1] = bl1Codes[_bitbuffer.range(0, 0)];
        symbol[2] = bl2Codes[_bitbuffer.range(0, 1)];
        symbol[3] = bl3Codes[_bitbuffer.range(0, 2)];
        symbol[4] = bl4Codes[_bitbuffer.range(0, 3)];
        symbol[5] = bl5Codes[_bitbuffer.range(0, 4)];
        symbol[6] = bl6Codes[_bitbuffer.range(0, 5)];
        symbol[7] = bl7Codes[_bitbuffer.range(0, 6)];

        auto current_val = symbol[current_bits];
        bitbuffer[0] = _bitbuffer >> current_bits;
        bitbuffer[1] = _bitbuffer >> extra;
        extra_copy = _bitbuffer & ((1 << extra) - 1);

        if (isExtra) {
            isExtra = false;
            copy += extra_copy;
            _bitbuffer = bitbuffer[1];
            bits_cntr -= extra;
        } else if (copy == 0) {
            if (current_val < 16) {
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                len = current_val;
                copy = 1;
            } else if (current_val == 16) {
                copy = 3;                  // use 2 bits
                _bitbuffer = bitbuffer[0]; // dump 2 bits
                bits_cntr -= current_bits; // update bits_cntr
                extra = 2;
                isExtra = true;
            } else if (current_val == 17) {
                len = 0;
                copy = 3; // use 3 bits
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                extra = 3;
                isExtra = true;
            } else {
                len = 0;
                copy = 11; // use 7 bits
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                extra = 7;
                isExtra = true;
            }
        }

        if (copy != 0) {
            lens[dynamic_curInSize++] = len;
            copy -= 1;
        }
        if (bits_cntr < 16 && !done) {
            uint16_t tmp_data = inStream.read();
            _bitbuffer += (bitBufferTypeLL)(tmp_data) << bits_cntr;
            done = inEos.read();
            bits_cntr += 16;
        }
    }
}

// Content of called function
void loadBitStreamLL(bitBufferTypeLL& bitbuffer,
                     ap_uint<6>& bits_cntr,
                     hls::stream<ap_uint<16> >& inStream,
                     hls::stream<bool>& inEos,
                     bool& done) {
    if (bits_cntr < 16 && (done == false)) {
    loadBitStream:
        uint16_t tmp_dt = (uint16_t)inStream.read();
        bitbuffer += (bitBufferTypeLL)(tmp_dt) << bits_cntr;
        done = inEos.read();
        bits_cntr += 16;
    }
}

// Content of called function
void loadBitStream(bitBufferType& bitbuffer,
                   ap_uint<6>& bits_cntr,
                   hls::stream<ap_uint<16> >& inStream,
                   hls::stream<bool>& inEos,
                   bool& done) {
#pragma HLS INLINE off
    while (bits_cntr < 32 && (done == false)) {
    loadBitStream:
        uint16_t tmp_dt = (uint16_t)inStream.read();
        bitbuffer += (bitBufferType)(tmp_dt) << bits_cntr;
        done = inEos.read();
        bits_cntr += 16;
    }
}

// Content of called function
void code_generator_array_dyn_new(uint8_t curr_table,
                                  ap_uint<5>* lens,
                                  ap_uint<9> codes,
                                  ap_uint<16>* codeOffsets,
                                  ap_uint<9>* bl1Codes,
                                  ap_uint<9>* bl2Codes,
                                  ap_uint<9>* bl3Codes,
                                  ap_uint<9>* bl4Codes,
                                  ap_uint<9>* bl5Codes,
                                  ap_uint<9>* bl6Codes,
                                  ap_uint<9>* bl7Codes,
                                  ap_uint<9>* bl8Codes,
                                  ap_uint<9>* bl9Codes,
                                  ap_uint<9>* bl10Codes,
                                  ap_uint<9>* bl11Codes,
                                  ap_uint<9>* bl12Codes,
                                  ap_uint<9>* bl13Codes,
                                  ap_uint<9>* bl14Codes,
                                  ap_uint<9>* bl15Codes) {
    uint16_t min = 15;
    uint16_t max = 0;

    const uint16_t c_maxbits = 15;

    ap_uint<9> count[c_maxbits + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = count

cnt_lens:
    for (ap_uint<9> i = 0; i < codes; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<5> val = lens[i];
        count[val]++;
    }

    count[0] = 0;
    ap_uint<15> firstCode[15];
#pragma HLS ARRAY_PARTITION variable = firstCode
    ap_uint<16> code = 0;
firstCode:
    for (uint16_t i = 1; i <= c_maxbits; i++) {
#pragma HLS PIPELINE II = 1
        code = (code + count[i - 1]) << 1;
        codeOffsets[i - 1] = code;
        firstCode[i - 1] = code;
    }

    uint16_t blen = 0;
CodeGen:
    for (uint16_t i = 0; i < codes; i++) {
#pragma HLS PIPELINE II = 1
        blen = lens[i];
        if (blen != 0) {
            switch (blen) {
                case 1:
                    bl1Codes[firstCode[0]] = i;
                    break;
                case 2:
                    bl2Codes[firstCode[1]] = i;
                    break;
                case 3:
                    bl3Codes[firstCode[2]] = i;
                    break;
                case 4:
                    bl4Codes[firstCode[3]] = i;
                    break;
                case 5:
                    bl5Codes[firstCode[4]] = i;
                    break;
                case 6:
                    bl6Codes[firstCode[5]] = i;
                    break;
                case 7:
                    bl7Codes[firstCode[6]] = i;
                    break;
                case 8:
                    bl8Codes[firstCode[7]] = i;
                    break;
                case 9:
                    bl9Codes[ap_uint<8>(firstCode[8])] = i;
                    break;
                case 10:
                    bl10Codes[ap_uint<8>(firstCode[9])] = i;
                    break;
                case 11:
                    bl11Codes[ap_uint<8>(firstCode[10])] = i;
                    break;
                case 12:
                    bl12Codes[ap_uint<8>(firstCode[11])] = i;
                    break;
                case 13:
                    bl13Codes[ap_uint<8>(firstCode[12])] = i;
                    break;
                case 14:
                    bl14Codes[ap_uint<8>(firstCode[13])] = i;
                    break;
                case 15:
                    bl15Codes[ap_uint<8>(firstCode[14])] = i;
                    break;
            }
            firstCode[blen - 1]++;
        }
    }
}

// Content of called function
void discardBitStreamLL(bitBufferTypeLL& bitbuffer, ap_uint<6>& bits_cntr, ap_uint<6> n_bits) {
#pragma HLS INLINE off
    bitbuffer >>= n_bits;
    bits_cntr -= n_bits;
}