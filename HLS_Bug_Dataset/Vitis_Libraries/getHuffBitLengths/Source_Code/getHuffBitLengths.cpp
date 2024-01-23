void getHuffBitLengths(hls::stream<IntVectorStream_dt<SYMBOL_BITS, 1> >& parentStream,
                       hls::stream<DSVectorStream_dt<Histogram, 1> >& lengthHistogramStream) {
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    ap_uint<1> metaIdx = 0;
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = parent type = ram_2p impl = lutram
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
    //#pragma HLS BIND_STORAGE variable = temp_array type = ram_t2p impl = bram
    Histogram length_histogram[c_lengthHistogram];
#pragma HLS ARRAY_PARTITION variable = length_histogram complete

create_tree_comp_bitlengths:
    while (true) {
        DSVectorStream_dt<Histogram, 1> outHistVal;
        uint16_t i_symbolSize = hctMeta[metaIdx++]; // current symbol size
        auto inPrtVal = parentStream.read();
        uint16_t heapLength = inPrtVal.data[0];
        // termination condition
        if (inPrtVal.strobe == 0) {
            outHistVal.strobe = 0;
            outHistVal.data[0] = 0;
            lengthHistogramStream << outHistVal;
            break;
        }
        auto rhpLen = heapLength + (uint16_t)(heapLength < 3);

    init_lenHist_parent_stream:
        for (uint16_t i = 0; i < i_symbolSize; ++i) {
#pragma HLS PIPELINE II = 1
            temp_array[i] = 0;
            if (i < rhpLen) {
                inPrtVal = parentStream.read();
                parent[i] = inPrtVal.data[0];
            } else {
                parent[i] = 0;
            }
        }
        // init histogram
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS UNROLL
            length_histogram[i] = 0;
        }
        // read the left & right to output stream
        constexpr uint8_t lrItr = 1 + ((SYMBOL_SIZE - 1) / SYMBOL_BITS);
        constexpr uint8_t bitRem = SYMBOL_SIZE - ((lrItr - 1) * SYMBOL_BITS);
    read_left_right:
        for (ap_uint<2> k = 0; k < 2; ++k) {
            inPrtVal = parentStream.read();
        read_word:
            for (uint8_t i = 0; i < lrItr - 1; ++i) {
#pragma HLS PIPELINE II = 1
                right >>= SYMBOL_BITS;
                right.range(SYMBOL_SIZE - 1, SYMBOL_SIZE - SYMBOL_BITS) = inPrtVal.data[0];
                // read next part
                inPrtVal = parentStream.read();
            }
            right >>= bitRem;
            right.range(SYMBOL_SIZE - 1, SYMBOL_SIZE - bitRem) = (ap_uint<bitRem>)inPrtVal.data[0];
            if (k == 0) left = right;
        }

        // get bit-lengths from tree
        computeBitLengthLL<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                      temp_array);
        // write the output
        outHistVal.strobe = 1;
    write_blen_hist:
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 18
#pragma HLS PIPELINE II = 1
            outHistVal.data[0] = length_histogram[i];
            lengthHistogramStream << outHistVal;
        }
    }
}

// Content of called function
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