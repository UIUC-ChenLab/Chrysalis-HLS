void bytePacker(hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> >& inStream,
                hls::stream<IntVectorStream_dt<8, DWIDTH / 8> >& outStream) {
    constexpr int incr = DWIDTH / 8;
    constexpr uint8_t factor = DWIDTH / 8;
    ap_uint<2 * DWIDTH> inputWindow;

    ap_uint<4> inputIdx = 0;
    bool packerDone = false;
    IntVectorStream_dt<8, DWIDTH / 8> outVal;
    outVal.strobe = incr;

multicorePacker:
    for (; packerDone == false;) {
#pragma HLS PIPELINE II = 1
        assert((inputIdx + factor) <= 2 * (DWIDTH / 8));
        ap_uint<SIZE_DWIDTH + DWIDTH> inVal = inStream.read();

        uint8_t inSize = inVal.range(SIZE_DWIDTH - 1, 0);
        packerDone = (inSize == 0);
        inputWindow.range((inputIdx + factor) * 8 - 1, inputIdx * 8) =
            inVal.range(SIZE_DWIDTH + DWIDTH - 1, SIZE_DWIDTH);

        inputIdx += inSize;

        for (uint16_t k = 0, j = 0; k < DWIDTH; k += incr) {
#pragma HLS UNROLL
            outVal.data[j++] = inputWindow.range(k + 7, k);
        }

        if (inputIdx >= factor) {
            outStream << outVal;
            inputWindow >>= DWIDTH;
            inputIdx -= factor;
        }
    }

    // writing last bytes
    if (inputIdx) {
        for (uint16_t i = 0, j = 0; i < DWIDTH; i += incr) {
#pragma HLS UNROLL
            outVal.data[j++] = inputWindow.range(i + 7, i);
        }
        outVal.strobe = inputIdx;
        outStream << outVal;
    }
    // send end of stream data
    outVal.strobe = 0;
    outStream << outVal;
}