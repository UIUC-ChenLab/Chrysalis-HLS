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