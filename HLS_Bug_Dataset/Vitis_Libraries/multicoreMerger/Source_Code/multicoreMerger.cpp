void multicoreMerger(hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> > inStream[NUM_BLOCKS],
                     hls::stream<ap_uint<DWIDTH> >& strdStream,
                     hls::stream<ap_uint<16> >& strdSizeStream,
                     hls::stream<ap_uint<DWIDTH> >& outStream,
                     hls::stream<bool>& outStreamEos,
                     hls::stream<uint32_t>& outSizeStream) {
    constexpr int incr = DWIDTH / 8;
    uint32_t outSize = 0;
    ap_uint<NUM_BLOCKS> is_pending;
    ap_uint<2 * DWIDTH> inputWindow;
    uint32_t factor = DWIDTH / 8;
    uint32_t inputIdx = 0;

    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
    }

    while (is_pending) {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            bool blockDone = false;
            bool not_first = false;
            for (; (blockDone == false) && (is_pending(i, i) == true);) {
#pragma HLS PIPELINE II = 1
                assert((inputIdx + factor) <= 2 * (DWIDTH / 8));

                ap_uint<DWIDTH + SIZE_DWIDTH> inVal = inStream[i].read();
                uint8_t inSize = inVal.range(SIZE_DWIDTH - 1, 0);
                outSize += inSize;
                inputWindow.range((inputIdx + factor) * 8 - 1, inputIdx * 8) =
                    inVal.range(SIZE_DWIDTH + DWIDTH - 1, SIZE_DWIDTH);

                ap_uint<DWIDTH> outData = inputWindow.range(DWIDTH - 1, 0);
                inputIdx += inSize;

                // checks per engine end of data or not, use inSize to avoid eos
                blockDone = (inSize == 0);
                is_pending.range(i, i) = (not_first || inSize > 0);

                if (inputIdx >= factor) {
                    outStream << outData;
                    outStreamEos << 0;
                    inputWindow >>= DWIDTH;
                    inputIdx -= factor;
                }
                not_first = true;
            }
        }
    }

    int16_t storedSize = strdSizeStream.read();
    outSize += storedSize;
    for (; storedSize > 0; storedSize -= factor) {
        // adding stored block header
        inputWindow.range((inputIdx + 1) * 8 - 1, inputIdx * 8) = 0;
        inputIdx += 1;
        inputWindow.range((inputIdx + 2) * 8 - 1, inputIdx * 8) = storedSize;
        inputIdx += 2;
        inputWindow.range((inputIdx + 2) * 8 - 1, inputIdx * 8) = ~storedSize;
        inputIdx += 2;
        outSize += 5;

        // process stored block data
        inputWindow.range((inputIdx + factor) * 8 - 1, inputIdx * 8) = strdStream.read();
        ap_uint<DWIDTH> outData = inputWindow.range(DWIDTH - 1, 0);
        inputIdx += factor;
        if (inputIdx >= factor) {
            outStream << outData;
            outStreamEos << 0;
            inputWindow >>= DWIDTH;
            inputIdx -= factor;
        }
    }

    if (outSize <= MIN_BLCK_SIZE) storedSize = strdSizeStream.read();

    if (inputIdx) {
        outStream << inputWindow.range(DWIDTH - 1, 0);
        outStreamEos << 0;
    }

    // send end of stream data
    outStream << 0;
    outStreamEos << 1;
    outSizeStream << outSize;
}