void simpleStreamDownSizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                           hls::stream<uint16_t>& inSizeStream,
                           hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream) {
    const int c_byteWidth = 8;
    const int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    const int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    const int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;

downsizer_top:
    for (uint16_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint16_t outSizeV = (inSize - 1) / c_outWord + 1;
    downsizer_assign:
        for (uint16_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
            int idx = itr % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<OUT_DATAWIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_DATAWIDTH - 1, idx * OUT_DATAWIDTH);
            outStream << tmpValue;
        }
    }
}