void simpleUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                   hls::stream<bool>& inStreamEos,
                   hls::stream<bool>& inFileEos,
                   hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                   hls::stream<bool>& outStreamEos,
                   hls::stream<uint32_t>& outSizeStream) {
    constexpr int c_byteWidth = 8;
    constexpr int c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr int c_wordSize = OUT_WIDTH / c_byteWidth;
    constexpr int c_size = BURST_SIZE * c_wordSize;

    while (1) {
        bool eosFile = inFileEos.read();
        if (eosFile == true) break;

        ap_uint<OUT_WIDTH> outBuffer = 0;
        uint32_t byteIdx = 0;
        uint16_t sizeWrite = 0;
        bool eos_flag = false;
    stream_upsizer:
        do {
#pragma HLS PIPELINE II = 1
            if (byteIdx == c_upsizeFactor) {
                outStream << outBuffer;
                outStreamEos << false;
                sizeWrite++;
                if (sizeWrite == BURST_SIZE) {
                    outSizeStream << c_size;
                    sizeWrite = 0;
                }
                byteIdx = 0;
            }
            ap_uint<IN_WIDTH> inValue = inStream.read();
            eos_flag = inStreamEos.read();
            outBuffer.range((byteIdx + 1) * IN_WIDTH - 1, byteIdx * IN_WIDTH) = inValue;
            byteIdx++;
        } while (eos_flag == false);

        if (byteIdx && (eosFile == false)) {
            outStream << outBuffer;
            outStreamEos << true;
            sizeWrite++;
            outSizeStream << (sizeWrite * c_wordSize);
        }
    }
    outSizeStream << 0;
}