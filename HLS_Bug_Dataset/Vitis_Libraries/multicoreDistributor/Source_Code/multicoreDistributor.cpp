void multicoreDistributor(hls::stream<ap_uint<DWIDTH> >& inStream,
                          hls::stream<uint32_t>& inSizeStream,
                          hls::stream<ap_uint<DWIDTH> >& strdStream,
                          hls::stream<ap_uint<16> >& strdSizeStream,
                          hls::stream<ap_uint<DWIDTH> > distStream[NUM_BLOCKS],
                          hls::stream<ap_uint<FREQ_DWIDTH> > distSizeStream[NUM_BLOCKS]) {
    constexpr int incr = DWIDTH / 8;
    ap_uint<4> core = 0;
    uint32_t readSize = 0;
    uint32_t inputSize = inSizeStream.read();

    while (readSize < inputSize) {
        core %= NUM_BLOCKS;
        ap_uint<FREQ_DWIDTH> outputSize = ((readSize + BLOCK_SIZE) > inputSize) ? (inputSize - readSize) : BLOCK_SIZE;

        bool storedBlockFlag = (outputSize <= MIN_BLCK_SIZE);

        readSize += outputSize;
        if (!storedBlockFlag)
            distSizeStream[core] << outputSize;
        else
            strdSizeStream << outputSize;

    multicoreDistributor:
        for (ap_uint<FREQ_DWIDTH> j = 0; j < outputSize; j += incr) {
#pragma HLS PIPELINE II = 1
            ap_uint<DWIDTH> outData = inStream.read();
            if (storedBlockFlag)
                strdStream << outData;
            else
                distStream[core] << outData;
        }
        core++;
    }

    for (int i = 0; i < NUM_BLOCKS; i++) {
        distSizeStream[i] << 0;
    }
    strdSizeStream << 0;
}