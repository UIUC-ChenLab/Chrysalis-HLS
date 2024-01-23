void reverseLitQuadStreams(hls::stream<ap_uint<68> >& inLitStream,
                           hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream,
                           hls::stream<ap_uint<68> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    constexpr uint16_t c_litBufSize = (BLOCK_SIZE / 32);
    // storing only 1/4th of the block size, 8-bytes a time
    // literal buffer
    ap_uint<64> litBuffer[c_litBufSize];
#pragma HLS BIND_STORAGE variable = litBuffer type = ram_t2p impl = bram
    ap_uint<64> outLit;
    ap_uint<68> outVal;

pre_proc_lit_main:
    while (true) {
        bool rdLitFlag = false;
        auto inVal = inLitStream.read();
        if (inVal.range(3, 0) == 0) break;
        uint8_t streamCnt = 1;
        uint16_t streamSize[5] = {0, 0, 0, 0, 0}; // 1 dummy entry
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
        outVal.range(3, 0) = 1;
        // first send the stream count
        outVal.range(11, 4) = streamCnt;
        outReversedLitStream << outVal;
    write_stream_sizes:
        for (uint8_t k = 0; k < streamCnt; ++k) {
#pragma HLS UNROLL
            outLit.range(15 + (k * 16), k * 16) = streamSize[k];
        }
        outVal.range(67, 4) = outLit;
        outReversedLitStream << outVal;
        // indexes
        uint16_t litRcnt = 0, litWcnt = 0;
        uint16_t wIdx = 1;
        uint16_t rIdx = 0;
        // write already read value into buffer
        litBuffer[0] = inVal.range(67, 4);
        ap_uint<64> prevWord = 0;
        uint8_t extBytesRead = 0;
        int8_t rwInc = 1;
        litWcnt = inVal.range(3, 0);
    rev_lit_loop_1to5:
        for (uint8_t si = 0; si < streamCnt + 1; ++si) {
            if (extBytesRead) {
                // write first byte to buffer
                litBuffer[wIdx] = prevWord;
                wIdx += rwInc;
                litWcnt = extBytesRead;
                // write first output for previous sub-stream
                outVal.range(3, 0) = 8 - extBytesRead;
                outLit = litBuffer[rIdx];
            // assign value in reverse byte order
            assign_outVal_1:
                for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                    uint8_t rK = 7 - k;
                    outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                }
                outReversedLitStream << outVal;
                rIdx += rwInc;
                litRcnt = outVal.range(3, 0);
            }
        rev_lit_loop_overlap:
            while ((si < streamCnt && litWcnt < streamSize[si]) || (si > 0 && litRcnt < streamSize[si - 1])) {
#pragma HLS PIPELINE II = 1
                // run till one of the conditions is true
                // buffer sub-stream
                if (si < streamCnt && litWcnt < streamSize[si]) {
                    inVal = inLitStream.read();
                    litBuffer[wIdx] = inVal.range(67, 4);
                    prevWord = inVal.range(67, 4);
                    litWcnt += 8; // to handle last stream size not aligned to 8 bytes case
                    wIdx += rwInc;
                }
                // read-out buffer data in reverse after first sub-stream is buffered
                if (si > 0 && litRcnt < streamSize[si - 1]) {
                    outLit = litBuffer[rIdx];
                    rIdx += rwInc;
                // assign value in reverse byte order
                assign_outVal_2:
                    for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                        uint8_t rK = 7 - k;
                        outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                    }
                    outVal.range(3, 0) = 8;
                    outReversedLitStream << outVal;
                    litRcnt += 8;
                }
            }
            // printf("Written %u bytes\n", wb);
            // get extra bytes for currently buffered stream
            extBytesRead = litWcnt - streamSize[si];
            // re-initialize literal index in sub-stream
            litWcnt = 0;
            litRcnt = 0;
            // manage direction in memory access
            // both indices move in same direction
            rwInc = (~rwInc) + 1; // flip +1/-1
            // manage memory access indices
            if ((uint8_t)(si & 1) != 0) { // odd, works only after type-casting to int types
                rIdx = wIdx + 1;
                wIdx = 0;
            } else { // even
                rIdx = wIdx - 1;
                wIdx = c_litBufSize - 1;
            }
        }
        // dump strobe 0 from input
        inVal = inLitStream.read();
        // end of block indicator not needed, since size is also sent
    }
    // end of data
    outVal.range(3, 0) = 0;
    outReversedLitStream << outVal;
}