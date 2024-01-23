void filterRadixSortPart1(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                          hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                          hls::stream<ap_uint<9> >& heapLenStream1,
                          hls::stream<ap_uint<9> >& heapLenStream2,
                          hls::stream<ap_uint<c_tgnSymbolBits> >& maxCodes,
                          hls::stream<bool>& isEOBlocks) {
    //#pragma HLS allocation function instances = _rdxsrtInitHistogram<SYMBOL_BITS> limit = 1
    // Filter input frequencies and partial radix sort (1st half)
    const ap_uint<9> hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_2p impl = lutram
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = prev_sorting type = ram_1p impl = lutram
    Digit current_digit[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = current_digit type = ram_1p impl = lutram
    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete

    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;
    ap_uint<1> metaIdx = 0;
filter_partsort_ldblock:
    while (!last) {
        // filter-sort for literals and distances
        ap_uint<9> i_symbolSize = hctMeta[metaIdx]; // current symbol size
        uint16_t heapLength = 0;

        ap_uint<c_tgnSymbolBits> smLen = 0;
        bool read_flag = false;
        auto curFreq = inFreqStream.read();
        // send oes stream once per lit-dist (2)trees
        last = (curFreq.strobe == 0);
        if (metaIdx == 0) isEOBlocks << last;
        if (last) break;

        // pre-initialize
        _rdxsrtInitHistogram<SYMBOL_BITS>(digit_histogram);

    filter_compute_histogram:
        for (uint16_t n = 0; n < i_symbolSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 30
            if (read_flag) curFreq = inFreqStream.read();
            auto cf = ((n == 256) ? (ap_uint<MAX_FREQ_DWIDTH>)1 : curFreq.data[0]);
            read_flag = true;
            // get non-zero frequencies
            if (cf > 0) {
                Symbol<MAX_FREQ_DWIDTH> cVal;
                cVal.value = smLen = n;
                cVal.frequency = cf;
                heap[heapLength] = cVal;
                prev_sorting[heapLength] = cVal;
                // create histogram
                Digit digit = (cVal.frequency & (RADIX - 1));
                current_digit[heapLength] = digit;
                ++digit_histogram[digit];
                ++heapLength;
            }
        }
        if (WRITE_MXC) maxCodes << smLen;

        _rdxsrtFindDigitLocation<SYMBOL_BITS>(digit_histogram, digit_location);

    re_sort_write_output1:
        for (uint16_t j = 0; j < heapLength; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            auto dlc = digit_location[digit];
            heap[dlc] = prev_sorting[j];
            digit_location[digit] = dlc + 1;
        }
        heapLenStream1 << heapLength;
        heapLenStream2 << heapLength;
        // send sorted frequencies
        hpVal.strobe = 1;
        for (uint16_t i = 0; i < heapLength; ++i) {
#pragma HLS PIPELINE II = 1
            hpVal.data[0] = heap[i];
            heapStream << hpVal;
        }
        // to prevent hang, send one 0 frequency value (invalid, used to detect 0 heapLength)
        if (heapLength == 0) {
            hpVal.data[0].frequency = 0;
            hpVal.data[0].value = 0;
            heapStream << hpVal;
        }
        // end of this chunk
        hpVal.strobe = 0;
        heapStream << hpVal;

        ++metaIdx;
    }
    // terminate
    hpVal.strobe = 0;
    heapStream << hpVal;
    heapLenStream1 << 0xFFF;
    heapLenStream2 << 0xFFF;
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
void _rdxsrtInitHistogram(ap_uint<SYMBOL_BITS> digit_histogram[RADIX]) {
#pragma HLS INLINE
init_histogram:
    for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS UNROLL
        digit_histogram[i] = 0;
    }
}