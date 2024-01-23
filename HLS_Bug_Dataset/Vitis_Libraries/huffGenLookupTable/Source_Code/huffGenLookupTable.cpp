void huffGenLookupTable(uint8_t* weights, HuffmanTable* huffTable, uint8_t accuracyLog, uint16_t weightCnt) {
    // create huffman lookup table
    // regenerate huffman tree using literal bit-lengths
    typedef ap_uint<MAX_CODELEN + 1> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_CODELEN + 1];
    ap_uint<32> bitlen_histogram[MAX_CODELEN + 1];
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
        bitlen_histogram[cblen]++;
        if (cblen > 0) cblen = (accuracyLog + 1 - cblen);
        bitlens[i] = cblen;
    }

    // generate first codes
    first_codeword[0] = bitlen_histogram[0];

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = 1; i < accuracyLog + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 8
        uint16_t cur = nextCode;
        nextCode += (bitlen_histogram[i] << (i - 1));
        first_codeword[i] = cur;
    }

hflkpt_codegen_outer:
    for (int i = 0; i < weightCnt; ++i) {
        uint32_t hfw = weights[i];
        const uint32_t len = (1 << hfw) >> 1;
        const auto fcw = first_codeword[hfw];
    hflkpt_codegen:
        for (uint16_t u = fcw; u < fcw + len; ++u) {
#pragma HLS PIPELINE II = 1
            huffTable[u].symbol = i;
            huffTable[u].bitlen = bitlens[i];
        }
        first_codeword[hfw] = fcw + len;
    }
}