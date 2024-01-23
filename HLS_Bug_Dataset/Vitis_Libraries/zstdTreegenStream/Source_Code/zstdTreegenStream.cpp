void zstdTreegenStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                       hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                       hls::stream<IntVectorStream_dt<4, 1> >& outWeightStream,
                       hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
#pragma HLS DATAFLOW
    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapStream("heapStream");
    hls::stream<ap_uint<9> > heapLenStream("heapLenStream");
    hls::stream<DSVectorStream_dt<Codeword, 1> > hufCodeStream("hufCodeStream");
    hls::stream<bool> eobStream("eobStream");
    hls::stream<bool> eoBlocks[2];

    details::zstdFreqFilterSort<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(inFreqStream, heapStream,
                                                                                          heapLenStream, eobStream);
    details::streamDistributor<2>(eobStream, eoBlocks);
    details::zstdGetHuffmanCodes<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH, MAX_BITS>(
        heapStream, heapLenStream, eoBlocks[0], hufCodeStream);
    details::huffCodeWeightDistributor<details::c_maxLitV + 1, MAX_FREQ_DWIDTH, MAX_BITS, 4>(
        hufCodeStream, eoBlocks[1], outCodeStream, outWeightStream, weightFreqStream);
}

// Content of called function
void zstdFreqFilterSort(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                        hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                        hls::stream<ap_uint<9> >& heapLenStream,
                        hls::stream<bool>& eobStream) {
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;

    hls::stream<ap_uint<SYMBOL_BITS> > maxCodes("maxCodes");
#pragma HLS STREAM variable = maxCodes depth = 2

filter_sort_ldblock:
    while (!last) {
        // filter-sort for literals
        ap_uint<9> i_symbolSize = SYMBOL_SIZE; // current symbol size
        uint16_t heapLength = 0;

        // filter the input, write 0 heapLength at end of block
        filter<MAX_FREQ_DWIDTH, 0>(inFreqStream, heap, &heapLength, maxCodes, i_symbolSize);
        // dump maxcode
        // maxCodes.read();
        // check for end of block
        last = (heapLength == 0xFFFF);
        eobStream << last;
        if (last) break;

        // sort the input
        radixSort<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

        // send sorted frequencies
        heapLenStream << heapLength;
        hpVal.strobe = 1;
        for (uint16_t i = 0; i < heapLength; i++) {
            hpVal.data[0] = heap[i];
            heapStream << hpVal;
        }
    }
}

// Content of called function
void radixSort(Symbol<MAX_FREQ_DWIDTH>* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];
    bool not_sorted = true;

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (uint8_t shift = 0; shift < MAX_FREQ_DWIDTH && not_sorted; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 5 avg = 4
    init_histogram:
        for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

        auto prev_freq = heap[0].frequency;
        not_sorted = false;
    compute_histogram:
        for (uint16_t j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
            Symbol<MAX_FREQ_DWIDTH> val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
            // check if not already in sorted order
            if (prev_freq > val.frequency) not_sorted = true;
            prev_freq = val.frequency;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (uint8_t i = 0; (i < RADIX - 1) && not_sorted; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (uint16_t j = 0; j < n && not_sorted; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

// Content of called function
void filter(Frequency<MAX_FREQ_DWIDTH>* inFreq,
            Symbol<MAX_FREQ_DWIDTH>* heap,
            uint16_t* heapLength,
            uint16_t* symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    uint16_t smLen = 0;
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        auto cf = inFreq[n];
        if (n == 256) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = 1;
            ++hpLen;
        } else if (cf != 0) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = cf;
            ++hpLen;
        }
    }

    heapLength[0] = hpLen;
    symLength[0] = smLen;
}

// Content of called function
void huffCodeWeightDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& hufCodeStream,
                               hls::stream<bool>& isEOBlocks,
                               hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                               hls::stream<IntVectorStream_dt<BLEN_BITS, 1> >& outWeightsStream,
                               hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
    // distribute huffman codes to multiple output streams and one separate bitlen stream
    DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> outCode;
    IntVectorStream_dt<BLEN_BITS, 1> outWeight;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outFreq;
    int blk_n = 0;
distribute_code_block:
    while (isEOBlocks.read() == false) {
        ++blk_n;
        ap_uint<MAX_FREQ_DWIDTH> weightFreq[MAX_BITS + 1];
        ap_uint<BLEN_BITS> blenBuf[SYMBOL_SIZE];
#pragma HLS ARRAY_PARTITION variable = weightFreq complete
#pragma HLS BIND_STORAGE variable = blenBuf type = ram_1p impl = lutram
        ap_uint<BLEN_BITS> curMaxBitlen = 0;
        uint8_t maxWeight = 0;
        uint16_t maxVal = 0;
    init_freq_bl:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE off
            weightFreq[i] = 0;
        }
        outCode.strobe = 1;
        outWeight.strobe = 1;
        outFreq.strobe = 1;
    distribute_code_loop:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto inCode = hufCodeStream.read();
            uint8_t hfblen = inCode.data[0].bitlength;
            uint16_t hfcode = inCode.data[0].codeword;
            outCode.data[0].code = hfcode;
            outCode.data[0].bitlen = hfblen;
            blenBuf[i] = hfblen;
            if (hfblen > curMaxBitlen) curMaxBitlen = hfblen;
            if (hfblen > 0) {
                maxVal = (uint16_t)i;
            }
            outCodeStream << outCode;
        }
    send_weights:
        for (ap_uint<9> i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto bitlen = blenBuf[i];
            auto blenWeight = (uint8_t)((bitlen > 0) ? (uint8_t)(curMaxBitlen + 1 - bitlen) : 0);
            outWeight.data[0] = blenWeight;
            weightFreq[blenWeight] += (uint8_t)(i < maxVal + 1); // conditional increment
            outWeightsStream << outWeight;
        }
        // write maxVal as first value
        outFreq.data[0] = maxVal;
        weightFreqStream << outFreq;
    // send weight frequencies
    send_weight_freq:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE II = 1
            outFreq.data[0] = weightFreq[i];
            weightFreqStream << outFreq;
            if (outFreq.data[0] > 0) maxWeight = i; // to be deduced by module reading this stream
        }
        // end of block
        outCode.strobe = 0;
        outWeight.strobe = 0;
        outFreq.strobe = 0;
        outCodeStream << outCode;
        outWeightsStream << outWeight;
        weightFreqStream << outFreq;
    }
    // end of all data
    outCode.strobe = 0;
    outWeight.strobe = 0;
    outFreq.strobe = 0;
    outCodeStream << outCode;
    outWeightsStream << outWeight;
    weightFreqStream << outFreq;
}

// Content of called function
void zstdGetHuffmanCodes(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                         hls::stream<ap_uint<9> >& heapLenStream,
                         hls::stream<bool>& isEOBlocks,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];
#pragma HLS ARRAY_PARTITION variable = length_histogram complete
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];

    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_t2p impl = bram
#pragma HLS AGGREGATE variable = heap

    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
construct_tree_ldblock:
    while (isEOBlocks.read() == false) {
        uint16_t heapLength = heapLenStream.read();
    init_buffers:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
#pragma HLS PIPELINE II = 1
            parent[i] = 0;
            if (i < c_lengthHistogram) length_histogram[i] = 0;
            temp_array[i] = 0;
            if (i < heapLength) {
                hpVal = heapStream.read();
                heap[i] = hpVal.data[0];
            }
            symbol_bits[i] = 0;
        }

        // create tree
        createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

        // get bit-lengths from tree
        computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                    temp_array);

        // truncate tree for any bigger bit-lengths
        truncateTree(length_histogram, c_lengthHistogram, MAX_BITS);

        // canonize the tree
        canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits, MAX_BITS + 1);

        // generate huffman codewords
        createZstdCodeword<MAX_BITS>(symbol_bits, length_histogram, outCodes, SYMBOL_SIZE, MAX_BITS, heapLength);
    }
}

// Content of called function
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

// Content of called function
void truncateTree(Histogram* length_histogram, uint16_t c_tree_depth, int max_bit_len) {
    int j = max_bit_len;
move_nodes:
    for (uint16_t i = c_tree_depth - 1; i > max_bit_len; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 572 max = 572 avg = 572
#pragma HLS PIPELINE II = 1
        // Look to see if there are any nodes at lengths greater than target depth
        int cnt = 0;
    reorder:
        for (; length_histogram[i] != 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 3 avg = 3
            if (j == max_bit_len) {
                // find the deepest leaf with codeword length < target depth
                --j;
            trctr_mv:
                while (length_histogram[j] == 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
                    --j;
                }
            }
            // Move leaf with depth i to depth j + 1
            length_histogram[j] -= 1;     // The node at level j is no longer a leaf
            length_histogram[j + 1] += 2; // Two new leaf nodes are attached at level j+1
            length_histogram[i - 1] += 1; // The leaf node at level i+1 gets attached here
            length_histogram[i] -= 2;     // Two leaf nodes have been lost from  level i

            // now deepest leaf with codeword length < target length
            // is at level (j+1) unless (j+1) == target length
            ++j;
        }
    }
}

// Content of called function
void createZstdCodeword(ap_uint<4>* symbol_bits,
                        Histogram* length_histogram,
                        hls::stream<DSVectorStream_dt<Codeword, 1> >& huffCodes,
                        uint16_t cur_symSize,
                        uint16_t cur_maxBits,
                        uint16_t symCnt) {
    //#pragma HLS inline
    bool allSameBlen = true;
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    DSVectorStream_dt<Codeword, 1> hfc;
    hfc.strobe = 1;

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
    uint8_t uniq_bl_idx = 0;
find_uniq_blen_count:
    for (uint8_t i = 0; i < cur_maxBits + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 12
        if (length_histogram[i] == cur_symSize) uniq_bl_idx = i;
    }
    // If only 1 uniq_blc for all symbols divide into 3 bitlens
    // Means, if all the bitlens are same(mainly bitlen-8) then use an alternate tree
    // Fix the current bitlength_histogram and symbol_bits so that it generates codes-bitlens for alternate tree
    if (uniq_bl_idx > 0) {
        length_histogram[7] = 1;
        length_histogram[9] = 2;
        length_histogram[8] -= 3;

        symbol_bits[0] = 7;
        symbol_bits[1] = 9;
        symbol_bits[2] = 9;
    }

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = cur_maxBits; i > 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 11
        uint16_t cur = nextCode;
        nextCode += (length_histogram[i]);
        nextCode >>= 1;
        first_codeword[i] = cur;
    }
    Codeword code;
assign_codewords_sm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 256 min = 256 avg = 256
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // length = (uniq_bl_idx > 0 && k > 2 && length > 8) ? 8 : length;	// not needed if treegen is optimal
        length = (symCnt == 0) ? 0 : length;
        code.codeword = (uint16_t)first_codeword[length];
        // get bitlength for code
        length = (symCnt == 0 && k == 0) ? 1 : length;
        code.bitlength = length;
        // write out codes
        hfc.data[0] = code;
        first_codeword[length]++;
        huffCodes << hfc;
    }
}

// Content of called function
void createTree(Symbol<MAX_FREQ_DWIDTH>* heap,
                int num_symbols,
                ap_uint<SYMBOL_BITS>* parent,
                ap_uint<SYMBOL_SIZE>& left,
                ap_uint<SYMBOL_SIZE>& right,
                Frequency<MAX_FREQ_DWIDTH>* frequency) {
    ap_uint<SYMBOL_BITS> tree_count = 0; // Number of intermediate node assigned a parent
    ap_uint<SYMBOL_BITS> in_count = 0;   // Number of inputs consumed
    ap_uint<SYMBOL_SIZE> tmp;
    left = 0;
    right = 0;

    // for case with less number of symbols
    if (num_symbols < 3) num_symbols++;
// this loop needs to run at least twice
create_heap:
    for (uint16_t i = 0; i < num_symbols; ++i) {
#pragma HLS PIPELINE II = 3
#pragma HLS LOOP_TRIPCOUNT min = 19 avg = 286 max = 286
        Frequency<MAX_FREQ_DWIDTH> node_freq = 0;
        Frequency<MAX_FREQ_DWIDTH> intermediate_freq = frequency[tree_count];
        Frequency<MAX_FREQ_DWIDTH> symFreq = heap[in_count].frequency;
        tmp = 1;
        tmp <<= i;

        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // left[i] = s.value;       // set input symbol value as left node
            node_freq = symFreq; // Add symbol frequency to total node frequency
            // move to the next input symbol
            ++in_count;
        } else {
            // pick internal node without a parent
            // left[i] = INTERNAL_NODE;           // Set symbol to indicate an internal node
            left |= tmp;
            node_freq = intermediate_freq; // Add child node frequency
            parent[tree_count] = i;        // Set this node as child's parent
            // Go to next parent-less internal node
            ++tree_count;
        }

        intermediate_freq = frequency[tree_count];
        symFreq = heap[in_count].frequency;
        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // right[i] = s.value;
            frequency[i] = node_freq + symFreq;
            ++in_count;
        } else {
            // Pick internal node without a parent
            // right[i] = INTERNAL_NODE;
            right |= tmp;
            frequency[i] = node_freq + intermediate_freq;
            parent[tree_count] = i;
            ++tree_count;
        }
    }
}

// Content of called function
void computeBitLength(ap_uint<SYMBOL_BITS>* parent,
                      ap_uint<SYMBOL_SIZE>& left,
                      ap_uint<SYMBOL_SIZE>& right,
                      int num_symbols,
                      Histogram* length_histogram,
                      Frequency<MAX_FREQ_DWIDTH>* child_depth) {
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
// this loop needs to run at least once
// II is 1 or 2 depending on configuration of memory
// used for arrays "child_depth" and "length_histogram"
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE
        tmp = 1;
        tmp <<= i;
        uint32_t length = child_depth[parent[i]] + 1;
        child_depth[i] = length;
        bool is_left_internal = ((left & tmp) == 0);
        bool is_right_internal = ((right & tmp) == 0);

        if ((is_left_internal || is_right_internal)) {
            uint32_t children = 1; // One child of the original node was a symbol
            if (is_left_internal && is_right_internal) {
                children = 2; // Both the children of the original node were symbols
            }
            length_histogram[length] += children;
        }
    }
}

// Content of called function
void streamDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 1) break;
    } while (1);
}