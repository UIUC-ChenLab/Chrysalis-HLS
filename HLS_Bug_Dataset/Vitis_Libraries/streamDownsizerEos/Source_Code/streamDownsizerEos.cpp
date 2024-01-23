void streamDownsizerEos(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                        hls::stream<uint32_t>& inSizeStream,
                        hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
                        hls::stream<bool>& outStreamEos) {
    constexpr int c_byteWidth = 8;
    constexpr int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    constexpr int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    constexpr int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;
    uint32_t inSize = inSizeStream.read();
    uint32_t outSizeV = (inSize - 1) / c_outWord + 1;

downsizer_assign:
    for (uint32_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
        int idx = itr % factor;
        if (idx == 0) inBuffer = inStream.read();
        ap_uint<OUT_DATAWIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_DATAWIDTH - 1, idx * OUT_DATAWIDTH);
        outStream << tmpValue;
        outStreamEos << false;
    }
    outStream << 0;
    outStreamEos << true;
}