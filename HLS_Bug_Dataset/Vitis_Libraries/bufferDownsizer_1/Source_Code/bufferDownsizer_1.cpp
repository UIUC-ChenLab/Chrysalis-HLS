void bufferDownsizer(hls::stream<ap_uint<72> >& inStream,
                     hls::stream<ap_uint<16> >& outStream,
                     hls::stream<bool>& outEos) {
    constexpr int16_t c_factor = 4;  // = 64 / 16
    constexpr int16_t c_outWord = 2; // = 16 / 2
    ap_uint<16> outVal;

    ap_uint<SWIDTH> dsize = 0;
    ap_uint<SWIDTH> cntr = 0;
    auto inVal = inStream.read();
    // proceed further if valid size
    ap_uint<SWIDTH> inSize = inVal.range(7, 0);
    // if (inSize == 0) break;
    auto outSizeV = ((inSize - 1) / c_outWord) + 1;
downsizer_assign:
    while (inSize > 0) {
#pragma HLS PIPELINE II = 1
        outVal = inVal.range(23, 8);
        inVal >>= 16;
        outStream << outVal;

        outEos << 0;
        dsize++;
        if (dsize == outSizeV) {
            inVal = inStream.read();
            inSize = inVal.range(7, 0);

            dsize = 0;
            outSizeV = ((inSize - 1) / c_outWord) + 1;
        }
    }
    // Block end Condition
    outEos << 1;
    outStream << 0;
}