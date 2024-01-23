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