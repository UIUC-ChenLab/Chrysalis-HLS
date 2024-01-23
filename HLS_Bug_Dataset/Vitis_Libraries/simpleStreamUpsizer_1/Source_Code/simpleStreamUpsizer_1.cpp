void simpleStreamUpsizer(hls::stream<IntVectorStream_dt<8, IN_WIDTH / 8> >& inStream,
                         hls::stream<ap_uint<OUT_WIDTH + SIZE_DWIDTH> >& outStream) {
    constexpr uint8_t c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr uint8_t c_inBytes = IN_WIDTH / 8;
    ap_uint<IN_WIDTH> inVal;
    ap_uint<OUT_WIDTH> outVal;
    bool last = false;
    ap_uint<4> dsize;

stream_upsizer_top:
    while (!last) {
        last = true;
        uint8_t byteIdx = 0;
        dsize = 0;
        auto inStVal = inStream.read();
        bool loop_continue = (inStVal.strobe != 0);
    stream_upsizer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            if (byteIdx == c_upsizeFactor) {
                ap_uint<SIZE_DWIDTH + OUT_WIDTH> tmpVal = outVal;
                tmpVal <<= SIZE_DWIDTH;
                tmpVal.range(SIZE_DWIDTH - 1, 0) = dsize;
                outStream << tmpVal;
                byteIdx = 0;
                dsize = 0;
                loop_continue = (inStVal.strobe != 0);
            }
        upszr_assign_input:
            for (uint8_t b = 0; b < c_inBytes; ++b) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 0 max = c_inBytes
                if (b < inStVal.strobe) inVal.range(((b + 1) * 8) - 1, b * 8) = inStVal.data[b];
            }
            outVal >>= IN_WIDTH;
            outVal.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = inVal;
            ++byteIdx;
            dsize += inStVal.strobe;
            if (inStVal.strobe != 0) inStVal = inStream.read();
        }
        // end of block/files
        outStream << 0;
    }
}