void zlibTreegenStreamWrapper(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77Tree[NUM_BLOCK],
                              hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[NUM_BLOCK]) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77SerialTree("lz77SerialTree");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufSerialCodeStream("hufSerialCodeStream");
    hls::stream<uint8_t> idxNum("idxNum");
#pragma HLS STREAM variable = lz77SerialTree depth = 4
#pragma HLS STREAM variable = hufSerialCodeStream depth = 4
#pragma HLS STREAM variable = idxNum depth = 32

    zlibTreegenScheduler<NUM_BLOCK, MAX_FREQ_DWIDTH>(lz77Tree, lz77SerialTree, idxNum);
    zlibTreegenStream<MAX_FREQ_DWIDTH, 0>(lz77SerialTree, hufSerialCodeStream);
    zlibTreegenDistributor<NUM_BLOCK>(hufCodeStream, hufSerialCodeStream, idxNum);
}

// Content of called function
void zlibTreegenDistributor(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[NUM_BLOCK],
                            hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufSerialCodeStream,
                            hls::stream<uint8_t>& inIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> hufCodeOut;

tgndst_units_main:
    for (uint8_t i = inIdxNum.read(); i != 0xFF; i = inIdxNum.read()) {
    tgndst_litDist:
        for (uint16_t j = 0; j < c_litDistCodeCnt; j++) {
#pragma HLS PIPELINE II = 1
            hufCodeOut = hufSerialCodeStream.read();
            hufCodeStream[i] << hufCodeOut;
        }
    tgndst_bitlen:
        while (1) {
#pragma HLS PIPELINE II = 1
            hufCodeOut = hufSerialCodeStream.read();
            hufCodeStream[i] << hufCodeOut;
            if (hufCodeOut.data[0].bitlen == 0) break;
        }
    }
    // send eos to all unit
    hufCodeOut.strobe = 0;
tgndst_send_eos:
    for (uint8_t i = 0; i < NUM_BLOCK; ++i) {
        hufCodeStream[i] << hufCodeOut;
    }
}

// Content of called function
void zlibTreegenStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& lz77TreeStream,
                       hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeStream) {
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes("ldCodes");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes1("ldCodes1");
    hls::stream<DSVectorStream_dt<Codeword, 1> > ldCodes2("ldCodes2");
    hls::stream<DSVectorStream_dt<Codeword, 1> > blCodes("blCodes");
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > ldFrequency("ldFrequency");

    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes("ldMaxCodes");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes1("ldMaxCodes1");
    hls::stream<ap_uint<c_tgnSymbolBits> > ldMaxCodes2("ldMaxCodes2");
    hls::stream<ap_uint<c_tgnSymbolBits> > blMaxCodes("blMaxCodes");

    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapStream("heapStream");
    hls::stream<ap_uint<9> > heapLenStream("heapLenStream");
    hls::stream<bool> isEOBlocks("eob_source");
    hls::stream<bool> eoBlocks[5];

#pragma HLS STREAM variable = heapStream depth = 320
#pragma HLS STREAM variable = ldCodes2 depth = 320
#pragma HLS STREAM variable = ldMaxCodes2 depth = 4
#pragma HLS STREAM variable = eoBlocks depth = 8

// DATAFLOW
#pragma HLS DATAFLOW
    huffConstructTreeStream_1<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(lz77TreeStream, heapStream,
                                                                                heapLenStream, ldMaxCodes, isEOBlocks);
    streamDistributor<5>(isEOBlocks, eoBlocks);
    huffConstructTreeStream_2<c_litCodeCount, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(heapStream, heapLenStream, eoBlocks[0],
                                                                                ldCodes);
    codeWordDistributor(ldCodes, ldCodes1, ldCodes2, ldMaxCodes, ldMaxCodes1, ldMaxCodes2, eoBlocks[1]);
    genBitLenFreq<MAX_FREQ_DWIDTH>(ldCodes1, eoBlocks[2], ldFrequency, ldMaxCodes1);
    processBitLength<MAX_FREQ_DWIDTH>(ldFrequency, eoBlocks[3], blCodes, blMaxCodes);
    sendTrees<SEND_EOS>(ldMaxCodes2, blMaxCodes, ldCodes2, blCodes, eoBlocks[4], hufCodeStream);
}

// Content of called function
void huffConstructTreeStream_1(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
                               hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                               hls::stream<ap_uint<9> >& heapLenStream,
                               hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                               hls::stream<bool>& isEOBlocks) {
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    bool last = false;
filter_sort_ldblock:
    while (!last) {
    // filter-sort for literals and distances
    filter_sort_litdist:
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
            uint16_t heapLength = 0;

            // filter the input, write 0 heapLength at end of block
            filter<MAX_FREQ_DWIDTH>(inFreq, heap, &heapLength, maxCodes, i_symbolSize);

            // check for end of block
            last = (heapLength == 0xFFFF);
            if (metaIdx == 0) isEOBlocks << last;
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
void sendTrees(hls::stream<ap_uint<c_tgnSymbolBits> >& maxLdCodes,
               hls::stream<ap_uint<c_tgnSymbolBits> >& maxBlCodes,
               hls::stream<DSVectorStream_dt<Codeword, 1> >& Ldcodes,
               hls::stream<DSVectorStream_dt<Codeword, 1> >& Blcodes,
               hls::stream<bool>& isEOBlocks,
               hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hfcodeStream) {
    const uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
#pragma HLS ARRAY_PARTITION variable = bitlen_vals complete dim = 1

    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outHufCode;
#pragma HLS AGGREGATE variable = outHufCode
send_tree_outer:
    while (isEOBlocks.read() == false) {
        Codeword zeroValue;
        zeroValue.bitlength = 0;
        zeroValue.codeword = 0;
        Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
#pragma HLS AGGREGATE variable = outCodes

        ap_uint<c_tgnSymbolBits> maxCodesReg[3];

        const uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
        // initialize all the memory
        maxCodesReg[0] = maxLdCodes.read();
        outHufCode.strobe = 1;
    read_litcodes_send:
        for (uint16_t i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }
        // last value write
        outCodes[c_litCodeCount] = zeroValue;
        maxCodesReg[1] = maxLdCodes.read();

    read_dstcodes_send:
        for (uint16_t i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[offsets[1] + i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }

        outCodes[c_litCodeCount + c_dstCodeCount + 1] = zeroValue;
        maxCodesReg[2] = maxBlCodes.read();

    read_blcodes:
        for (uint16_t i = offsets[2]; i < offsets[2] + c_blnCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Blcodes.read();
            outCodes[i] = ldc.data[0];
        }

        ap_uint<c_tgnSymbolBits> bl_mxc;
        bool mxb_continue = true;
    bltree_blen:
        for (bl_mxc = c_blnCodeCount - 1; (bl_mxc >= 3) && mxb_continue; --bl_mxc) {
#pragma HLS PIPELINE II = 1
            auto cIdx = offsets[2] + bitlen_vals[bl_mxc];
            mxb_continue = (outCodes[cIdx].bitlength == 0);
        }

        maxCodesReg[2] = bl_mxc + 1;

        // Code from Huffman Encoder
        //********************************************//
        // Start of block = 4 and len = 3
        // For dynamic tree
        outHufCode.strobe = 1;
        outHufCode.data[0].code = 4;
        outHufCode.data[0].bitlen = 3;
        hfcodeStream << outHufCode;
        // lcodes
        outHufCode.data[0].code = ((maxCodesReg[0] + 1) - 257);
        outHufCode.data[0].bitlen = 5;
        hfcodeStream << outHufCode;
        // dcodes
        outHufCode.data[0].code = ((maxCodesReg[1] + 1) - 1);
        outHufCode.data[0].bitlen = 5;
        hfcodeStream << outHufCode;
        // blcodes
        outHufCode.data[0].code = ((maxCodesReg[2] + 1) - 4);
        outHufCode.data[0].bitlen = 4;
        hfcodeStream << outHufCode;

        ap_uint<c_tgnSymbolBits> bitIndex = offsets[2];
        outHufCode.strobe = 1;
    // Send BL length data
    send_bltree:
        for (ap_uint<c_tgnSymbolBits> rank = 0; rank < maxCodesReg[2] + 1; ++rank) {
#pragma HLS LOOP_TRIPCOUNT avg = 19 max = 19
#pragma HLS PIPELINE II = 1
            outHufCode.data[0].code = outCodes[bitIndex + bitlen_vals[rank]].bitlength;
            outHufCode.data[0].bitlen = 3;
            hfcodeStream << outHufCode;
        } // BL data copy loop

        Codeword temp;
#pragma HLS AGGREGATE variable = temp
    // Send Bitlengths for Literal and Distance Tree
    send_ld_trees_outer:
        for (int tree = 0; tree < 2; ++tree) {
            uint8_t prevlen = 0;                                 // Last emitted Length
            uint8_t curlen = 0;                                  // Length of Current Code
            uint8_t nextlen = outCodes[offsets[tree]].bitlength; // Length of next code
            uint8_t count = 0;
            int max_count = 7; // Max repeat count
            int min_count = 4; // Min repeat count

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            uint16_t max_code = maxCodesReg[tree];
            temp = outCodes[bitIndex + c_reusePrevBlen];
            uint16_t reuse_prev_code = temp.codeword;
            uint8_t reuse_prev_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen];
            uint16_t reuse_zero_code = temp.codeword;
            uint8_t reuse_zero_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen7];
            uint16_t reuse_zero7_code = temp.codeword;
            uint8_t reuse_zero7_len = temp.bitlength;

            uint8_t repCnt = 0;
            bool refCntFlag = true;
        send_ldtree_main:
            for (uint16_t n = 0; n <= max_code || repCnt > 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 3
                if (repCnt != 0) {
                    outHufCode.data[0].code = temp.codeword;
                    outHufCode.data[0].bitlen = temp.bitlength;
                write_rep_codes_blens:
                    for (uint8_t k = 0; k < 3; ++k) {
#pragma HLS UNROLL
                        if (repCnt > 0) {
                            --repCnt;
                            hfcodeStream << outHufCode;
                        }
                    }
                    refCntFlag = (repCnt == 0);
                } else {
                    refCntFlag = true;
                    curlen = nextlen;
                    // Length of next code
                    nextlen = outCodes[offsets[tree] + n + 1].bitlength;

                    if (++count < max_count && curlen == nextlen) {
                        refCntFlag = false;
                    } else if (count < min_count) {
                        temp = outCodes[bitIndex + curlen];
                        repCnt = count;
                        refCntFlag = false;
                    } else if (curlen != 0) {
                        if (curlen != prevlen) {
                            temp = outCodes[bitIndex + curlen];
                            outHufCode.data[0].code = temp.codeword;
                            outHufCode.data[0].bitlen = temp.bitlength;
                            hfcodeStream << outHufCode;
                            count--;
                        }
                        outHufCode.data[0].code = reuse_prev_code;
                        outHufCode.data[0].bitlen = reuse_prev_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 3;
                        outHufCode.data[0].bitlen = 2;
                        hfcodeStream << outHufCode;

                    } else if (count <= 10) {
                        outHufCode.data[0].code = reuse_zero_code;
                        outHufCode.data[0].bitlen = reuse_zero_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 3;
                        outHufCode.data[0].bitlen = 3;
                        hfcodeStream << outHufCode;
                    } else {
                        outHufCode.data[0].code = reuse_zero7_code;
                        outHufCode.data[0].bitlen = reuse_zero7_len;
                        hfcodeStream << outHufCode;

                        outHufCode.data[0].code = count - 11;
                        outHufCode.data[0].bitlen = 7;
                        hfcodeStream << outHufCode;
                    }
                    ++n;
                }
                if (refCntFlag) {
                    count = 0;
                    prevlen = curlen;
                    if (nextlen == 0) {
                        max_count = 138, min_count = 3;
                    } else if (curlen == nextlen) {
                        max_count = 6, min_count = 3;
                    } else {
                        max_count = 7, min_count = 4;
                    }
                }
            }
        }
        // ends huffman stream for each block, strobe eos not needed from this module
        outHufCode.data[0].bitlen = 0;
        hfcodeStream << outHufCode;
    }
    if (SEND_EOS > 0) {
        // end of huffman tree data for all blocks
        outHufCode.strobe = 0;
        hfcodeStream << outHufCode;
    }
}

// Content of called function
void processBitLength(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& frequencies,
                      hls::stream<bool>& isEOBlocks,
                      hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                      hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes) {
    // read freqStream and generate codes for it
    // construct the huffman tree and generate huffman codes
    details::huffConstructTreeStream<c_blnCodeCount, c_tgnBitlengthBits, 15, MAX_FREQ_DWIDTH>(frequencies, isEOBlocks,
                                                                                              outCodes, maxCodes, 2);
}

// Content of called function
void huffConstructTreeStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
                             hls::stream<bool>& isEOBlocks,
                             hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes,
                             hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                             uint8_t metaIdx) {
#pragma HLS inline
    // construct huffman tree and generate codes and bit-lengths
    const ap_uint<9> hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                                      {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                                      {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};

    const ap_uint<9> i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
    const ap_uint<9> i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
    const ap_uint<9> i_maxBits = hctMeta[metaIdx][2];    // current max bits

    while (isEOBlocks.read() == false) {
        // internal buffers
        Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];

        ap_uint<SYMBOL_SIZE> left = 0;
        ap_uint<SYMBOL_SIZE> right = 0;
        ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
        Histogram length_histogram[LENGTH_SIZE];
        Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
        ap_uint<4> symbol_bits[SYMBOL_SIZE];
        IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> curFreq;
        uint16_t heapLength = 0;

        {
#pragma HLS dataflow
        init_buffers:
            for (ap_uint<9> i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE off
                parent[i] = 0;
                if (i < LENGTH_SIZE) length_histogram[i] = 0;
                temp_array[i] = 0;
                symbol_bits[i] = 0;
                /*heap[i].value = 0;
                heap[i].frequency = 0;*/
            }
            // filter the input, write 0 heapLength at end of block
            filter<MAX_FREQ_DWIDTH>(inFreq, heap, &heapLength, maxCodes, i_symbolSize);
        }

        // sort the input
        radixSort_1<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

        // create tree
        createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

        // get bit-lengths from tree
        computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                    temp_array);

        // truncate tree for any bigger bit-lengths
        truncateTree(length_histogram, LENGTH_SIZE, i_maxBits);

        // canonize the tree
        canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits, LENGTH_SIZE);

        // generate huffman codewords
        createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
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
void radixSort_1(Symbol<MAX_FREQ_DWIDTH>* heap, uint16_t n) {
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
        for (ap_uint<5> i = 0; i < RADIX; ++i) {
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
void genBitLenFreq(Codeword* outCodes, Frequency<MAX_FREQ_DWIDTH>* blFreq, uint16_t maxCode) {
    //#pragma HLS inline
    // generate bit-length frequencies using literal and distance bit-lengths
    ap_uint<4> tree_len[c_litCodeCount];

copy_blens:
    for (int i = 0; i <= maxCode; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = (uint8_t)outCodes[i].bitlength;
    }
clear_rem_blens:
    for (int i = maxCode + 2; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
        tree_len[i] = 0;
    }
    tree_len[maxCode + 1] = (uint8_t)0xff;

    int16_t prevlen = -1;
    int16_t curlen = 0;
    int16_t count = 0;
    int16_t max_count = 7;
    int16_t min_count = 4;
    int16_t nextlen = tree_len[0];

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

parse_tdata:
    for (uint32_t n = 0; n <= maxCode; ++n) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 3
        curlen = nextlen;
        nextlen = tree_len[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            blFreq[curlen] += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) {
                blFreq[curlen]++;
            }
            blFreq[c_reusePrevBlen]++;
        } else if (count <= 10) {
            blFreq[c_reuseZeroBlen]++;
        } else {
            blFreq[c_reuseZeroBlen7]++;
        }

        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
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

// Content of called function
void codeWordDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& inStreamCode,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode1,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode2,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& inStreamMaxCode,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode1,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode2,
                         hls::stream<bool>& isEOBlocks) {
    const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    while (isEOBlocks.read() == false) {
    distribute_litdist_codes:
        for (uint8_t i = 0; i < 2; i++) {
            auto maxCode = inStreamMaxCode.read();
            outStreamMaxCode1 << maxCode;
            outStreamMaxCode2 << maxCode;
        distribute_hufcodes_main:
            for (uint16_t j = 0; j < hctMeta[i]; j++) {
#pragma HLS PIPELINE II = 1
                auto inVal = inStreamCode.read();
                outStreamCode1 << inVal;
                outStreamCode2 << inVal;
            }
        }
    }
}

// Content of called function
void huffConstructTreeStream_2(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                               hls::stream<ap_uint<9> >& heapLenStream,
                               hls::stream<bool>& isEOBlocks,
                               hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = parent type = ram_2p impl = lutram
    Histogram length_histogram[c_lengthHistogram];

    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = temp_array type = ram_1wnr impl = bram
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_t2p impl = bram
#pragma HLS AGGREGATE variable = heap
    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    const ap_uint<9> hctMeta[3][3] = {{c_litCodeCount, c_litCodeCount, c_maxCodeBits},
                                      {c_dstCodeCount, c_dstCodeCount, c_maxCodeBits},
                                      {c_blnCodeCount, c_blnCodeCount, c_maxBLCodeBits}};
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
construct_tree_ldblock:
    while (isEOBlocks.read() == false) {
    construct_tree_litdist:
        for (uint8_t metaIdx = 0; metaIdx < 2; metaIdx++) {
            uint16_t i_symbolSize = hctMeta[metaIdx][0]; // current symbol size
            uint16_t i_treeDepth = hctMeta[metaIdx][1];  // current tree depth
            uint16_t i_maxBits = hctMeta[metaIdx][2];    // current max bits

            uint16_t heapLength = heapLenStream.read();

        init_buffers:
            for (uint16_t i = 0; i < i_symbolSize; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
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
            computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength,
                                                                        length_histogram, temp_array);

            // truncate tree for any bigger bit-lengths
            truncateTree(length_histogram, c_lengthHistogram, i_maxBits);

            // canonize the tree
            canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits,
                                                       c_lengthHistogram);

            // generate huffman codewords
            createCodeword<c_tgnMaxBits>(symbol_bits, length_histogram, outCodes, i_symbolSize, i_maxBits, heapLength);
        }
    }
}

// Content of called function
void zlibTreegenScheduler(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77InTree[NUM_BLOCK],
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lz77OutTree[NUM_TREEGEN],
                          hls::stream<ap_uint<4> >& coreIdxStream,
                          hls::stream<uint8_t>& outIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    ap_uint<NUM_BLOCK> is_pending;
    bool eos_tmp[NUM_BLOCK];
    static uint8_t treeIdx = 0;
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
        eos_tmp[i] = false;
    }
    bool read_flag = true;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> inVal;
    ap_uint<4> core = coreIdxStream.read();
    while (is_pending) {
        for (uint8_t i = core; i < NUM_BLOCK + core; i++) {
            ap_uint<4> j = i % NUM_BLOCK;
            if (eos_tmp[j] == false) {
                inVal = lz77InTree[j].read();
                read_flag = false;
            }
            bool eos = eos_tmp[j] ? eos_tmp[j] : (inVal.strobe == 0);
            is_pending.range(j, j) = eos ? 0 : 1;
            eos_tmp[j] = eos;
            if (!eos) {
                outIdxNum << j;
                for (uint16_t k = 0; k < c_litDistCodeCnt; k++) {
                    if (read_flag) inVal = lz77InTree[j].read();
                    lz77OutTree[treeIdx] << inVal;
                    read_flag = true;
                }
                treeIdx++;
                if (treeIdx == NUM_TREEGEN) treeIdx = 0;
            }
        }
    }
    inVal.strobe = 0;
    for (auto i = 0; i < NUM_TREEGEN; i++) {
        lz77OutTree[i] << inVal;
    }
    outIdxNum << 0xFF;
}