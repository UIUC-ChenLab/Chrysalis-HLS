void multicoreDistributor(hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> >& inStream,
                          hls::stream<SIZE_DT>& fileSizeStream,
                          hls::stream<ap_uint<DWIDTH> >& strdStream,
                          hls::stream<ap_uint<16> >& strdSizeStream,
                          hls::stream<bool>& blckEosStream,
                          hls::stream<ap_uint<4> >& coreIdxStream,
                          hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> > distStream[NUM_BLOCKS]) {
    constexpr int c_incr = DWIDTH / 8;
    constexpr int c_factor = (MIN_BLCK_SIZE + c_incr) / c_incr;
    constexpr int c_storedDepth = 2 * c_factor;
    static ap_uint<4> core = 0;
    coreIdxStream << core;

    SIZE_DT readSize = 0;
    SIZE_DT writeSize = 0;
    ap_uint<16> strdCntr = 0;
    ap_uint<SIZE_DWIDTH> strobe = 0;
    bool last = false;

    hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> > storedBufferStream;
#pragma HLS STREAM variable = storedBufferStream depth = c_storedDepth

init_loop:
    for (uint8_t i = 0; i < c_factor && !last; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<DWIDTH + SIZE_DWIDTH> tmpVal = inStream.read();
        strobe = tmpVal.range(SIZE_DWIDTH - 1, 0);
        last = (strobe == 0);
        readSize += strobe;
        storedBufferStream << tmpVal;
    }

distributor:
    while (!last) {
        core %= NUM_BLOCKS;
        blckEosStream << false;

        for (uint32_t i = 0; i < BLOCK_SIZE && !last; i += c_incr) {
#pragma HLS PIPELINE II = 1
            ap_uint<DWIDTH + SIZE_DWIDTH> tmpVal = inStream.read();
            ap_uint<SIZE_DWIDTH> strobe = tmpVal.range(SIZE_DWIDTH - 1, 0);
            readSize += strobe;
            last = (strobe == 0);
            ap_uint<DWIDTH + SIZE_DWIDTH> tmp = storedBufferStream.read();
            distStream[core] << tmp;
            strdCntr += tmp.range(SIZE_DWIDTH - 1, 0);
            writeSize += tmp.range(SIZE_DWIDTH - 1, 0);
            storedBufferStream << tmpVal;
        }

        if (!last) {
            distStream[core] << 0;
            strdCntr = 0;
            core++;
        }
    }

    blckEosStream << true;
    bool onlyOnce = true;
    bool endBlck = true;
    for (; writeSize != readSize && strdCntr != 0;) {
#pragma HLS PIPELINE II = 1
        ap_uint<SIZE_DWIDTH + DWIDTH> tmpVal = storedBufferStream.read();
        distStream[core] << tmpVal;
        strdCntr += tmpVal.range(SIZE_DWIDTH - 1, 0);
        writeSize += tmpVal.range(SIZE_DWIDTH - 1, 0);
        if (strdCntr == BLOCK_SIZE) {
            endBlck = false;
            strdCntr = 0;
        }
    }

    if (!endBlck) {
        if (readSize == writeSize)
            distStream[core] << storedBufferStream.read();
        else
            distStream[core] << 0;
        core++;
    } else if (endBlck && strdCntr > 0) {
        distStream[core] << storedBufferStream.read();
        core++;
    }

    for (; (writeSize != readSize && strdCntr == 0) || onlyOnce;) {
#pragma HLS PIPELINE II = 1
        if (onlyOnce) {
            strdSizeStream << (readSize - writeSize);
            onlyOnce = false;
            if (readSize == writeSize) break;
        }
        ap_uint<SIZE_DWIDTH + DWIDTH> tmpVal = storedBufferStream.read();
        strdStream << tmpVal.range(SIZE_DWIDTH + DWIDTH - 1, SIZE_DWIDTH);
        writeSize += tmpVal.range(SIZE_DWIDTH - 1, 0);
        if (readSize == writeSize) storedBufferStream.read();
    }

    // Total Input Size for GZIP only
    if (STRATEGY == 0) fileSizeStream << writeSize;

ip_terminate_data:
    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
        distStream[i] << 0;
    }
}