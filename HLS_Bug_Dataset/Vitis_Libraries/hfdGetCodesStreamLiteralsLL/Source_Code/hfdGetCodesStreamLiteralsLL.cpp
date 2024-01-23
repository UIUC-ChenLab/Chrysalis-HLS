void hfdGetCodesStreamLiteralsLL(uint16_t* decSize,
                                 uint8_t accuracyLog,
                                 uint8_t streamCnt,
                                 uint16_t weightCnt,
                                 uint8_t* weights,
                                 hls::stream<IntVectorStream_dt<BS_WIDTH, 1> >& huffBitStream,
                                 hls::stream<ap_uint<8> >& validBitCntStream,
                                 hls::stream<ap_uint<17> >& literalStream) {
    const uint16_t c_HBFSize = 48;
    const uint16_t c_BSWidth = BS_WIDTH;
    const int c_bsPB = c_BSWidth / 8;

    ap_uint<11> validCodeOffset[MAX_CODELEN + 1];
    ap_uint<4> bitLen[MAX_CODELEN + 1];
    ap_uint<8> symbol[2][MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = validCodeOffset complete dim = 0
#pragma HLS ARRAY_PARTITION variable = bitLen complete dim = 0
#pragma HLS ARRAY_PARTITION variable = symbol complete dim = 0

    // New huffman code
    ap_uint<MAX_CODELEN + 1> codeOffsets[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = codeOffsets dim = 1 complete

    ap_uint<8> bl1Code[2];
    ap_uint<8> bl2Code[4];
    ap_uint<8> bl3Code[8];
    ap_uint<8> bl4Code[16];
    ap_uint<8> bl5Code[32];
    ap_uint<8> bl6Code[64];
    ap_uint<8> bl7Code[128];
    ap_uint<8> bl8Code[256];
    ap_uint<8> bl9Code[256];
    ap_uint<8> bl10Code[256];
    ap_uint<8> bl11Code[256];
#pragma HLS BIND_STORAGE variable = bl1Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl2Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl3Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl4Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl5Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl6Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl7Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl8Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl9Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl10Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl11Code type = ram_2p impl = lutram

    // generate codes
    code_generator<MAX_CODELEN>(weights, codeOffsets, bl1Code, bl2Code, bl3Code, bl4Code, bl5Code, bl6Code, bl7Code,
                                bl8Code, bl9Code, bl10Code, bl11Code, accuracyLog, weightCnt);

    ap_uint<17> outBuffer;
decode_huff_bitstream_outer:
    for (ap_uint<3> si = 0; si < streamCnt; ++si) {
        // generate decompressed bytes from huffman encoded stream
        ap_uint<c_HBFSize> acchbs = 0;
        ap_uint<7> bitcnt = 0;

        bitcnt = validBitCntStream.read();
        // input register
        auto inValue = huffBitStream.read();
        acchbs.range(c_BSWidth - 1, 0) = inValue.data[0];
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (c_HBFSize - bitcnt);
        const int c_msbBitCnt = c_HBFSize - c_BSWidth;

        ap_uint<11> parallelAcc[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = parallelAcc complete
        ap_uint<8> tmpBitLen = bitcnt;
        bool readUpdFlag = ((tmpBitLen < 22) && (inValue.strobe == 1));
        uint8_t shiftCnt = c_msbBitCnt - tmpBitLen;

        ap_uint<c_HBFSize> tmp = 0;
        if (readUpdFlag) {
            inValue = huffBitStream.read();
            tmp = inValue.data[0];
            bitcnt += c_BSWidth;
        }
        ap_uint<c_HBFSize> acchbs1 = acchbs;
        ap_uint<c_HBFSize> acchbs2 = tmp << shiftCnt;
        acchbs = acchbs1 | acchbs2;

        // pre-read one data
        inValue = huffBitStream.read();

    // decode huffman bitstreams
    huf_dec_bitstream:
        for (uint32_t outBytes = 0; outBytes < decSize[si]; outBytes += 2) {
#pragma HLS PIPELINE II = 1

            for (ap_uint<4> i = 0; i < MAX_CODELEN + 1; ++i) {
#pragma HLS UNROLL
                parallelAcc[i] = acchbs.range(c_HBFSize - 1 - i, c_HBFSize - MAX_CODELEN - i);
                for (ap_uint<4> j = 0; j < MAX_CODELEN; ++j) {
#pragma HLS UNROLL
                    validCodeOffset[i].range(j, j) = (parallelAcc[i].range(10, 10 - j) >= codeOffsets[j]) ? 1 : 0;
                }
                bitLen[i] = 1 + ap_uint<6>(__builtin_ctz((unsigned int)(validCodeOffset[i])));
            }

            ap_uint<5> totalBitLen = bitLen[0] + bitLen[bitLen[0]];
            ap_uint<7> tmpBitLen = bitcnt - totalBitLen;
            ap_uint<5> shiftCnt = c_msbBitCnt - tmpBitLen;
            if (tmpBitLen < 22) {
                tmp = inValue.data[0];
                bitcnt = c_BSWidth + tmpBitLen;
            } else {
                tmp = 0;
                bitcnt = tmpBitLen;
            }
            ap_uint<c_HBFSize> acchbs1 = acchbs << totalBitLen;
            ap_uint<c_HBFSize> acchbs2 = tmp << shiftCnt;
            acchbs = acchbs1 | acchbs2;

            bool readUpdFlag = ((tmpBitLen < 22) && (inValue.strobe == 1));
            if (readUpdFlag) {
                inValue = huffBitStream.read();
            }
            symbol[0][1] = bl1Code[parallelAcc[0].range(10, 10)];
            symbol[0][2] = bl2Code[parallelAcc[0].range(10, 9)];
            symbol[0][3] = bl3Code[parallelAcc[0].range(10, 8)];
            symbol[0][4] = bl4Code[parallelAcc[0].range(10, 7)];
            symbol[0][5] = bl5Code[parallelAcc[0].range(10, 6)];
            symbol[0][6] = bl6Code[parallelAcc[0].range(10, 5)];
            symbol[0][7] = bl7Code[parallelAcc[0].range(10, 4)];
            symbol[0][8] = bl8Code[parallelAcc[0].range(10, 3)];
            symbol[0][9] = bl9Code[ap_uint<8>(parallelAcc[0].range(10, 2))];
            symbol[0][10] = bl10Code[ap_uint<8>(parallelAcc[0].range(10, 1))];
            symbol[0][11] = bl11Code[ap_uint<8>(parallelAcc[0].range(10, 0))];

            symbol[1][1] = bl1Code[parallelAcc[bitLen[0]].range(10, 10)];
            symbol[1][2] = bl2Code[parallelAcc[bitLen[0]].range(10, 9)];
            symbol[1][3] = bl3Code[parallelAcc[bitLen[0]].range(10, 8)];
            symbol[1][4] = bl4Code[parallelAcc[bitLen[0]].range(10, 7)];
            symbol[1][5] = bl5Code[parallelAcc[bitLen[0]].range(10, 6)];
            symbol[1][6] = bl6Code[parallelAcc[bitLen[0]].range(10, 5)];
            symbol[1][7] = bl7Code[parallelAcc[bitLen[0]].range(10, 4)];
            symbol[1][8] = bl8Code[parallelAcc[bitLen[0]].range(10, 3)];
            symbol[1][9] = bl9Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 2))];
            symbol[1][10] = bl10Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 1))];
            symbol[1][11] = bl11Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 0))];

            outBuffer.range(7, 0) = symbol[0][bitLen[0]];
            outBuffer.range(15, 8) = symbol[1][bitLen[bitLen[0]]];
            outBuffer.range(16, 16) = true;

            // write the literal to output stream
            literalStream << outBuffer;
        }
    }

    outBuffer = 0;
    literalStream << outBuffer;
}

// Content of called function
void code_generator(uint8_t* weights,
                    ap_uint<MAX_CODELEN + 1>* codeOffsets,
                    ap_uint<8>* bl1Codes,
                    ap_uint<8>* bl2Codes,
                    ap_uint<8>* bl3Codes,
                    ap_uint<8>* bl4Codes,
                    ap_uint<8>* bl5Codes,
                    ap_uint<8>* bl6Codes,
                    ap_uint<8>* bl7Codes,
                    ap_uint<8>* bl8Codes,
                    ap_uint<8>* bl9Codes,
                    ap_uint<8>* bl10Codes,
                    ap_uint<8>* bl11Codes,
                    uint8_t accuracyLog,
                    uint16_t weightCnt) {
    // regenerate huffman tree using literal bit-lengths
    typedef ap_uint<MAX_CODELEN + 1> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_CODELEN + 1];
    ap_uint<8> bitlen_histogram[MAX_CODELEN + 1];
    ap_uint<4> bitlens[256];
#pragma HLS ARRAY_PARTITION variable = first_codeword complete
#pragma HLS ARRAY_PARTITION variable = bitlen_histogram complete

    uint16_t codes[256];
// initialize first_codeword and bitlength histogram
hflkpt_init_blen_hist:
    for (uint8_t i = 0; i < MAX_CODELEN + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAX_CODELEN max = MAX_CODELEN
        bitlen_histogram[i] = 0;
    }
// read bit-lengths
hflkpt_fill_blen_hist:
    for (uint16_t i = 0; i < weightCnt; ++i) {
#pragma HLS PIPELINE II = 1
        // convert weight to bitlen
        uint8_t cblen = weights[i];
        if (cblen > 0) cblen = (accuracyLog + 1 - cblen);
        bitlens[i] = cblen;
        bitlen_histogram[cblen]++;
    }

    // generate first codes
    first_codeword[0] = 0;
    codeOffsets[0] = 0;

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (int8_t i = accuracyLog - 1; i >= 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 11
        uint16_t cur = nextCode;
        nextCode += (bitlen_histogram[i + 1]);
        nextCode >>= 1;
        first_codeword[i] = cur;
        codeOffsets[i] = cur;
    }

    uint16_t blen = 0;
CodeGen:
    for (uint16_t i = 0; i < weightCnt; i++) {
#pragma HLS PIPELINE II = 1
        blen = bitlens[i];
        if (blen != 0) {
            switch (blen) {
                case 1:
                    bl1Codes[first_codeword[0]] = i;
                    break;
                case 2:
                    bl2Codes[first_codeword[1]] = i;
                    break;
                case 3:
                    bl3Codes[first_codeword[2]] = i;
                    break;
                case 4:
                    bl4Codes[first_codeword[3]] = i;
                    break;
                case 5:
                    bl5Codes[first_codeword[4]] = i;
                    break;
                case 6:
                    bl6Codes[first_codeword[5]] = i;
                    break;
                case 7:
                    bl7Codes[first_codeword[6]] = i;
                    break;
                case 8:
                    bl8Codes[first_codeword[7]] = i;
                    break;
                case 9:
                    bl9Codes[ap_uint<8>(first_codeword[8])] = i;
                    break;
                case 10:
                    bl10Codes[ap_uint<8>(first_codeword[9])] = i;
                    break;
                case 11:
                    bl11Codes[ap_uint<8>(first_codeword[10])] = i;
                    break;
                default:
                    assert(0);
                    break;
            }
            first_codeword[blen - 1]++;
        }
    }
}