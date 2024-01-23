void simpleStreamUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                         hls::stream<bool>& inStreamEos,
                         hls::stream<uint32_t>& inSizeStream,
                         hls::stream<bool>& inFileEos,
                         hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                         hls::stream<bool>& outStreamEos,
                         hls::stream<ap_uint<4> >& outSizeStream) {
    constexpr int c_byteWidth = 8;
    constexpr int c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr int factor = IN_WIDTH / 8;
    uint32_t upsizerCntr = 0;

    while (1) {
        bool eosFile = inFileEos.read();
        if (eosFile == true) break;

        ap_uint<OUT_WIDTH> outBuffer = 0;
        uint8_t byteIdx = 0;
        uint32_t readSize = 0;
        bool eos_flag = false;
    stream_upsizer:
        do {
#pragma HLS PIPELINE II = 1
            if (byteIdx == c_upsizeFactor) {
                readSize += byteIdx * factor;
                outSizeStream << (byteIdx * factor);
                outStream << outBuffer;
                outStreamEos << false;
                byteIdx = 0;
            }
            ap_uint<IN_WIDTH> inValue = inStream.read();
            eos_flag = inStreamEos.read();
            outBuffer.range((byteIdx + 1) * IN_WIDTH - 1, byteIdx * IN_WIDTH) = inValue;
            byteIdx++;
        } while (eos_flag == false);

        uint32_t blockSize = inSizeStream.read();
        uint8_t leftBytes = blockSize - readSize;

        if (byteIdx && (eosFile == false)) {
            outSizeStream << leftBytes;
            outStream << outBuffer;
            outStreamEos << false;
        }
        // send dummy data to indicate end of each block
        outSizeStream << 0;
        outStream << 0;
        outStreamEos << 0;
    }

    outSizeStream << 0;
    outStream << 0;
    outStreamEos << 1;
}