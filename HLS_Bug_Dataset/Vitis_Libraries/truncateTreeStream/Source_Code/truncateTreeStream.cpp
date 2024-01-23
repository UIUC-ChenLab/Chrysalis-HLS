void truncateTreeStream(hls::stream<DSVectorStream_dt<Histogram, 1> >& inLengthHistogramStream,
                        hls::stream<DSVectorStream_dt<Histogram, 1> >& outLengthHistogramStream) {
    const uint16_t c_maxBitLen[2] = {c_blnCodeCount, c_blnCodeCount};
    Histogram length_histogram[c_lengthHistogram];
    //#pragma HLS ARRAY_PARTITION variable = length_histogram type = complete
    ap_uint<1> metaIdx = 0;
    DSVectorStream_dt<Histogram, 1> histVal;
trunc_tree_outer:
    while (true) {
        int max_bit_len = c_maxBitLen[metaIdx++];
        histVal = inLengthHistogramStream.read();
        length_histogram[0] = histVal.data[0];
        if (histVal.strobe == 0) {
            // exit
            outLengthHistogramStream << histVal;
            break;
        }
    read_blen_hist:
        for (uint8_t i = 1; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 17 max = 17
#pragma HLS PIPELINE II = 1
            histVal = inLengthHistogramStream.read();
            length_histogram[i] = histVal.data[0];
        }
        int j = max_bit_len;
    move_nodes:
        for (uint16_t i = c_lengthHistogram - 1; i > max_bit_len; --i) {
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
        histVal.strobe = 1;
    write_trc_blen_hist:
        for (uint8_t i = 0; i < c_lengthHistogram; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 18
#pragma HLS PIPELINE II = 1
            histVal.data[0] = length_histogram[i];
            outLengthHistogramStream << histVal;
        }
    }
}