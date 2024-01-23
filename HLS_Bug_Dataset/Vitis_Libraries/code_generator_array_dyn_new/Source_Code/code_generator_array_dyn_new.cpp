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