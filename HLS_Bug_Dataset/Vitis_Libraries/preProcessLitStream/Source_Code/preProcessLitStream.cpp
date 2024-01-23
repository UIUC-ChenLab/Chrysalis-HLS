void preProcessLitStream(hls::stream<IntVectorStream_dt<8, 1> >& inLitStream,
                         hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream,
                         hls::stream<IntVectorStream_dt<8, 1> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    constexpr uint16_t c_litBufSize = (BLOCK_SIZE / 4);
    // literal buffer
    ap_uint<8> litBuffer[c_litBufSize];
#pragma HLS BIND_STORAGE variable = litBuffer type = ram_t2p impl = bram
    IntVectorStream_dt<8, 1> outLit;

pre_proc_lit_main:
    while (true) {
        bool rdLitFlag = false;
        auto inVal = inLitStream.read();
        if (inVal.strobe == 0) break;
        uint8_t streamCnt = 1;
        uint16_t streamSize[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSize complete
        ap_uint<MAX_FREQ_DWIDTH> litCnt = litCntStream.read();
        // get out stream count and sizes
        if (litCnt > 255) {
            streamCnt = 4;
            streamSize[0] = 1 + (litCnt - 1) / 4;
            streamSize[1] = streamSize[0];
            streamSize[2] = streamSize[0];
            streamSize[3] = litCnt - (streamSize[0] * 3);
        } else {
            streamSize[0] = litCnt;
            streamSize[3] = litCnt;
        }
        outLit.strobe = 1;
        // first send the stream count
        outLit.data[0] = streamCnt;
        outReversedLitStream << outLit;
    write_stream_sizes:
        for (uint8_t k = 0; k < streamCnt; ++k) {
#pragma HLS PIPELINE II = 2
            outLit.data[0] = (uint8_t)(streamSize[k]);
            outReversedLitStream << outLit;
            outLit.data[0] = (uint8_t)(streamSize[k] >> 8);
            outReversedLitStream << outLit;
        }
        // write already read value into buffer
        uint16_t litIdx = 0;
        uint16_t wIdx = 1;
        uint16_t rIdx = 0;
        litBuffer[0] = inVal.data[0];
    rev_lit_loop_1:
        for (litIdx = 1; litIdx < streamSize[0]; ++litIdx) {
#pragma HLS PIPELINE II = 1
            inVal = inLitStream.read();
            litBuffer[wIdx++] = inVal.data[0];
        }
        // init read-write indices
        rIdx = wIdx - 1;
        wIdx = c_litBufSize - 1;

        int8_t rwInc = -1;
    rev_lit_loop_234:
        for (uint8_t i = 0; i < 3 && streamCnt > 1; ++i) {
        rev_lit_loop_overlap:
            for (litIdx = 0; litIdx < streamSize[0]; ++litIdx) {
#pragma HLS PIPELINE II = 1
                // stream out previously read data
                outLit.data[0] = litBuffer[rIdx];
                outReversedLitStream << outLit;
                rIdx += rwInc;
                // buffer next sub-stream
                if (litIdx < streamSize[i + 1]) {
                    inVal = inLitStream.read();
                    litBuffer[wIdx] = inVal.data[0];
                    wIdx += rwInc;
                }
            }
            // manage direction in memory access
            // both indices move in same direction
            rwInc = (~rwInc) + 1; // flip +1/-1
            // manage memory access indices
            if ((uint8_t)(i & 1) == 0) { // even, works only after type-casting to int types
                rIdx = wIdx + 1;
                wIdx = 0;
            } else { // odd
                rIdx = wIdx - 1;
                wIdx = c_litBufSize - 1;
            }
        }

    rev_lit_loop_5:
        for (litIdx = 0; litIdx < streamSize[3]; ++litIdx) {
#pragma HLS PIPELINE II = 1
            // stream out previously read data
            outLit.data[0] = litBuffer[rIdx];
            outReversedLitStream << outLit;
            rIdx += rwInc;
        }
        // dump strobe 0 from input
        inVal = inLitStream.read();
        // end of block indicator not needed, since size is also sent
    }
    // end of data
    outLit.strobe = 0;
    outReversedLitStream << outLit;
}