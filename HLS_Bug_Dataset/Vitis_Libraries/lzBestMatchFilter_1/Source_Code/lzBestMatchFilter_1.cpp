void lzBestMatchFilter(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                       hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    const int c_max_match_length = MATCH_LEN;

    IntVectorStream_dt<32, 1> compare_window;
    IntVectorStream_dt<32, 1> outStreamValue;

    while (true) {
        auto nextVal = inStream.read();
        if (nextVal.strobe == 0) {
            outStreamValue.strobe = 0;
            outStream << outStreamValue;
            break;
        }
        // assuming that, at least bytes more than LEFT_BYTES will be present at the input
        compare_window = nextVal;
        nextVal = inStream.read();

    lz_bestMatchFilter:
        for (; nextVal.strobe != 0;) {
#pragma HLS PIPELINE II = 1
            // shift register logic
            IntVectorStream_dt<32, 1> outValue = compare_window;
            compare_window = nextVal;
            nextVal = inStream.read();

            uint8_t match_length = outValue.data[0].range(15, 8);
            bool best_match = 1;
            // Find Best match
            compressd_dt compareValue = compare_window.data[0];
            uint8_t compareLen = compareValue.range(15, 8);
            if (match_length < compareLen) {
                best_match = 0;
            }
            if (best_match == 0) {
                outValue.data[0].range(15, 8) = 0;
                outValue.data[0].range(31, 16) = 0;
            }
            outStream << outValue;
        }
        outStream << compare_window;
        outStreamValue.strobe = 0;
        outStream << outStreamValue;
    }
}