void canonizeTree(Symbol<MAX_FREQ_DWIDTH>* sorted,
                  uint32_t num_symbols,
                  Histogram* length_histogram,
                  ap_uint<4>* symbol_bits,
                  uint16_t i_treeDepth) {
    int16_t length = i_treeDepth;
    ap_uint<SYMBOL_BITS> count = 0;
// Iterate across the symbols from lowest frequency to highest
// Assign them largest bit length to smallest
process_symbols:
    for (uint32_t k = 0; k < num_symbols; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 256 avg = 286
        if (count == 0) {
            // find the next non-zero bit length k
            count = length_histogram[--length];
        canonize_inner:
            while (count == 0 && length >= 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 avg = 1 max = 2
#pragma HLS PIPELINE II = 1
                // n  is the number of symbols with encoded length k
                count = length_histogram[--length];
            }
        }
        if (length < 0) break;
        symbol_bits[sorted[k].value] = length; // assign symbol k to have length bits
        --count;                               // keep assigning i bits until we have counted off n symbols
    }
}