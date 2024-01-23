void computeBitLengthLL(ap_uint<SYMBOL_BITS>* parent,
                        ap_uint<SYMBOL_SIZE>& left,
                        ap_uint<SYMBOL_SIZE>& right,
                        int num_symbols,
                        Histogram* length_histogram,
                        Frequency<MAX_FREQ_DWIDTH>* child_depth) {
    //#pragma HLS INLINE
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
    auto prevParent = parent[num_symbols - 2];
    auto length = child_depth[prevParent];
    ++length;
// this loop needs to run at least once
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 301
#pragma HLS pipeline II = 1
        auto pIdx = parent[i];
        if (pIdx != prevParent) {
            // use previously written value to avoid dependence
            length = 1 + (pIdx == (i + 1) ? length : child_depth[pIdx]);
            prevParent = pIdx;
        } else {
            tmp = 1;
            tmp <<= i;
            child_depth[i] = length;
            bool is_left_internal = ((left & tmp) == 0);
            bool is_right_internal = ((right & tmp) == 0);

            if ((is_left_internal || is_right_internal)) {
                length_histogram[length] += (1 + (uint8_t)(is_left_internal & is_right_internal));
            }
            --i;
        }
    }
}