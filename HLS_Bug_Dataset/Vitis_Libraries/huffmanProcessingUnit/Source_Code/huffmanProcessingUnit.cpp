void huffmanProcessingUnit(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                           hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    enum HuffEncoderStates { READ_LITERAL, READ_MATCH_LEN, READ_OFFSET0, READ_OFFSET1 };
    IntVectorStream_dt<32, 1> outValue;
    outValue.strobe = 0;
    bool done = false;
    int blk_n = 0;
    while (true) { // iterate over multiple blocks in a file
        enum HuffEncoderStates next_state = READ_LITERAL;

        IntVectorStream_dt<9, 1> nextValue = inStream.read();
        // end of file
        if (nextValue.strobe == 0) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }
        outValue.data[0] = 0;
        outValue.strobe = 1;

        done = false;
    hf_proc_static_main:
        while (!done) { // iterate over data in block
#pragma HLS PIPELINE II = 1
            bool outFlag = false;
            ap_uint<9> inValue = nextValue.data[0];
            bool tokenFlag = (inValue.range(8, 8) == 1);
            nextValue = inStream.read();

            if (next_state == READ_LITERAL) {
                if (tokenFlag) {
                    outValue.data[0].range(7, 0) = 0;
                    outValue.data[0].range(15, 8) = inValue.range(7, 0);
                    outFlag = false;
                    next_state = READ_OFFSET0;
                } else {
                    outValue.data[0].range(7, 0) = inValue;
                    outFlag = true;
                    next_state = READ_LITERAL;
                }
            } else if (next_state == READ_OFFSET0) {
                outFlag = false;
                outValue.data[0].range(23, 16) = inValue;
                outValue.strobe++;
                next_state = READ_OFFSET1;
            } else if (next_state == READ_OFFSET1) {
                outFlag = true;
                outValue.data[0].range(31, 24) = inValue;
                outValue.strobe++;
                next_state = READ_LITERAL;
            }

            if (outFlag) {
                outStream << outValue;
                outValue.data[0] = 0;
                outFlag = false;
            }
            // exit condition
            if (nextValue.strobe == 0) {
                done = true;
            }
        }
        // end of block
        outValue.strobe = 0;
        outStream << outValue;
    }
}