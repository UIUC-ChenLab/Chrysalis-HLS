void createTreeWrapper(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                       hls::stream<ap_uint<9> >& heapLenStream,
                       hls::stream<IntVectorStream_dt<SYMBOL_BITS, 1> >& parentStream) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = temp_array type = ram_t2p impl = bram
    bool done = false;
    IntVectorStream_dt<SYMBOL_BITS, 1> outPrtVal;
    outPrtVal.strobe = 1;

create_tree_comp_bitlengths:
    while (true) {
        uint16_t heapLength = heapLenStream.read();
        done = (heapLength == (((uint16_t)1 << 9) - 1));
        // termination condition
        if (done) {
            outPrtVal.strobe = 0;
            outPrtVal.data[0] = 0;
            parentStream << outPrtVal;
            break;
        }
        // write heapLength
        outPrtVal.data[0] = heapLength;
        parentStream << outPrtVal;
        // create tree
        createTreeStream<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heapStream, heapLength, parentStream, left, right,
                                                                    temp_array);
        // write the left & right to output stream
        constexpr uint8_t itrCnt = 1 + ((SYMBOL_SIZE - 1) / SYMBOL_BITS);
    write_left_right:
        for (ap_uint<2> k = 0; k < 2; ++k) {
            if (k == 1) left = right;
        write_word:
            for (uint8_t i = 0; i < itrCnt; ++i) {
#pragma HLS PIPELINE II = 1
                outPrtVal.data[0] = left.range(SYMBOL_BITS - 1, 0);
                left >>= SYMBOL_BITS;
                parentStream << outPrtVal;
            }
        }
    }
}