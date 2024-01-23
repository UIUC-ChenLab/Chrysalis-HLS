void canonizeGetCodes(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                      hls::stream<ap_uint<9> >& heapLenStream,
                      hls::stream<DSVectorStream_dt<Histogram, 1> >& lengthHistogramStream,
                      hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    static bool initBitsMem = true;
    // internal buffers
    static ap_uint<4> symbol_bits[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];
    ap_uint<1> metaIdx = 0;
    DSVectorStream_dt<Codeword, 1> cdVal;
    if (initBitsMem) { // init only once
    init_smb_buffer:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS LOOP_TRIPCOUNT max = SYMBOL_SIZE
#pragma HLS PIPELINE off
            symbol_bits[i] = 0;
        }
        initBitsMem = false;
    }
construct_tree_ldblock:
    while (true) {
        uint16_t i_symbolSize = hctMeta[metaIdx++]; // current symbol size
        uint16_t heapLength = heapLenStream.read(); // value dumped at end of file
        auto histVal = lengthHistogramStream.read();
        if (histVal.strobe == 0) break;
        // init latency will hide behind latency of previous module in dataflow

        length_histogram[0] = histVal.data[0];
    read_cnz_ln_hist_buff:
        for (uint8_t i = 1; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 17 max = 17
#pragma HLS PIPELINE II = 1
            auto histVal = lengthHistogramStream.read();
            length_histogram[i] = histVal.data[0];
        }

        // canonize the tree
        canonizeTreeStream<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heapStream, heapLength, length_histogram, symbol_bits,
                                                         c_lengthHistogram);

        // generate huffman codewords
        createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, c_maxCodeBits, heapLength);
    }
}

// Content of called function
void createCodeword(ap_uint<4>* symbol_bits,
                    Histogram* length_histogram,
                    hls::stream<DSVectorStream_dt<Codeword, 1> >& huffCodes,
                    uint16_t cur_symSize,
                    uint16_t cur_maxBits,
                    uint16_t symCnt) {
    //#pragma HLS inline
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    DSVectorStream_dt<Codeword, 1> hfc;
    hfc.strobe = 1;

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
first_codewords:
    for (uint16_t i = 1; i <= cur_maxBits; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 7 max = 15
#pragma HLS PIPELINE II = 1
        first_codeword[i] = (first_codeword[i - 1] + length_histogram[i - 1]) << 1;
    }
    Codeword code;
assign_codewords_sm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 286 avg = 286
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // if symbol has 0 bits, it doesn't need to be encoded
        LCL_Code_t out_reversed = first_codeword[length];
        out_reversed.reverse();
        out_reversed = out_reversed >> (MAX_LEN - length);

        hfc.data[0].codeword = (length == 0 || symCnt == 0) ? (uint16_t)0 : (uint16_t)out_reversed;
        length = (symCnt == 0) ? 0 : length;
        hfc.data[0].bitlength = (symCnt == 0 && k == 0) ? 1 : length;
        first_codeword[length]++;
        huffCodes << hfc;
        // reset symbol bits
        symbol_bits[k] = 0;
    }
}