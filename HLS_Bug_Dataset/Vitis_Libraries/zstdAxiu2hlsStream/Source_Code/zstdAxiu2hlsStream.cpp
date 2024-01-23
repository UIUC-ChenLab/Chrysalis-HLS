void zstdAxiu2hlsStream(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                        hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> >& outHlsStream) {
    constexpr uint8_t c_keepDWidth = IN_DWIDTH / 8;
    IntVectorStream_dt<8, c_keepDWidth> outVal;
    outVal.strobe = c_keepDWidth;
    auto inAxiVal = inStream.read();
axi_to_hls:
    for (; inAxiVal.last == false; inAxiVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outHlsStream << outVal;
    }
    uint8_t strb = countSetBits<c_keepDWidth>((int)(inAxiVal.keep));
    if (strb) { // write last byte if valid
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outVal.strobe = strb;
        outHlsStream << outVal;
    }
    outVal.strobe = 0;
    outHlsStream << outVal;
}

// Content of called function
uint8_t countSetBits(ap_uint<DWIDTH> val) {
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < DWIDTH; ++i) {
#pragma HLS UNROLL
        cnt += val.range(i, i);
    }
    return cnt;
}