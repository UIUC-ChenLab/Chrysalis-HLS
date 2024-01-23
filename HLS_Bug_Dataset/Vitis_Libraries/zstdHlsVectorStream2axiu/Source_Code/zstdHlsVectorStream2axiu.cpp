void zstdHlsVectorStream2axiu(hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> >& hlsInStream,
                              hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    constexpr uint8_t c_bytesInWord = OUT_DWIDTH / 8;
    ap_axiu<OUT_DWIDTH, 0, 0, 0> outVal;
    ap_uint<OUT_DWIDTH * 2> outReg;
    uint8_t bytesInReg = 0;
    outVal.keep = -1;
    outVal.last = false;
    uint32_t outCnt = 0;
hls_to_axi:
    for (auto inVal = hlsInStream.read(); inVal.strobe > 0; inVal = hlsInStream.read()) {
#pragma HLS PIPELINE II = 1
        // Write data to output register
        for (uint8_t i = 0; i < c_bytesInWord; ++i) {
#pragma HLS UNROLL
            uint16_t idx = (bytesInReg + i) * 8;
            outReg.range(idx + 7, idx) = inVal.data[i];
        }
        bytesInReg += inVal.strobe;
        outCnt += inVal.strobe;
        if (bytesInReg > c_bytesInWord - 1) {
            outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
            outStream << outVal;
            outReg >>= OUT_DWIDTH;
            bytesInReg -= c_bytesInWord;
        }
    }
    if (bytesInReg) {
        outVal.keep = (((uint16_t)1 << bytesInReg) - 1);
        outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
        outStream << outVal;
    }
    // send eos
    outVal.last = true;
    outVal.keep = 0;
    outVal.data = 0;
    outStream << outVal;
}