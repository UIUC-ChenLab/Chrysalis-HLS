void zstdHuffMultiBitPacker(
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, PARALLEL_HUFFMAN> >& hfEncodedStream,
    hls::stream<ap_uint<MAX_BITS * PARALLEL_HUFFMAN> >& hfBitStream,
    hls::stream<ap_uint<8> >& hfBitlenStream) {
    assert(PARALLEL_HUFFMAN == 2 || PARALLEL_HUFFMAN == 4 || PARALLEL_HUFFMAN == 8);
    // pack huffman codes from multiple input code streams
    constexpr uint8_t c_hf2cWidth = MAX_BITS * 2;
    constexpr uint8_t c_hf4cWidth = MAX_BITS * 4;
    constexpr uint8_t c_hfbsWidth = MAX_BITS * 8;
    bool done = false;
    ap_uint<c_hfbsWidth> outReg = 0;
    uint8_t cblen = 0;
hf_bitpacking_outer:
    while (!done) {
        done = true;
        uint16_t outCnt = 0;
        uint8_t remBits = 0;

    hf_bitPacking:
        for (auto inVal = hfEncodedStream.read(); inVal.strobe > 0; inVal = hfEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            done = false;
            // compute values for stage 1 of combination
            ap_uint<c_hf2cWidth> vec1[PARALLEL_HUFFMAN / 2];
            uint8_t blen1[PARALLEL_HUFFMAN / 2];
#pragma HLS ARRAY_PARTITION variable = vec1 type = complete
#pragma HLS ARRAY_PARTITION variable = blen1 type = complete
            for (uint8_t k = 0; k < PARALLEL_HUFFMAN / 2; ++k) {
#pragma HLS UNROLL
                vec1[k] = ((ap_uint<c_hf2cWidth>)inVal.data[(2 * k) + 1].code << inVal.data[2 * k].bitlen) +
                          inVal.data[2 * k].code;
                blen1[k] = inVal.data[(2 * k) + 1].bitlen + inVal.data[2 * k].bitlen;
            }
            // compute for stage 2
            ap_uint<c_hf4cWidth> v44[2] = {0, 0};
            uint8_t blen2[2] = {0, 0};
#pragma HLS ARRAY_PARTITION variable = v44 type = complete
#pragma HLS ARRAY_PARTITION variable = blen2 type = complete
            if (PARALLEL_HUFFMAN > 2) {
                for (uint8_t k = 0; k < PARALLEL_HUFFMAN / 4; ++k) {
#pragma HLS UNROLL
                    v44[k] = ((ap_uint<c_hf4cWidth>)vec1[(2 * k) + 1] << blen1[2 * k]) + vec1[2 * k];
                    blen2[k] = blen1[(2 * k) + 1] + blen1[2 * k];
                }
            }
            // compute for stage 3, final word
            if (PARALLEL_HUFFMAN > 4) {
                outReg = ((ap_uint<c_hfbsWidth>)v44[1] << blen2[0]) + v44[0];
                cblen = blen2[1] + blen2[0];
            } else if (PARALLEL_HUFFMAN > 2) {
                outReg = v44[0];
                cblen = blen2[0];
            } else {
                outReg = vec1[0];
                cblen = blen1[0];
            }
            // send output, if valid bits present
            if (cblen > 0) {
                hfBitStream << outReg;
                hfBitlenStream << cblen;
            }
            {
                // output bytes calculation, for debugging purposes
                uint8_t ob = (remBits + cblen) >> 3;
                outCnt += ob;
                remBits = remBits + cblen - (ob * 8);
            }
        }
        if (!done) {
            // mark end by adding 1-bit "1"
            outReg = 1;
            cblen = 1;
            hfBitStream << outReg;
            hfBitlenStream << cblen;
            {
                // output bytes calculation, for debugging purposes
                uint8_t ob = (remBits + cblen + 7) >> 3;
                outCnt += ob;
            }
        }
        // strobe 0, end of block
        hfBitlenStream << 0;
    }
}