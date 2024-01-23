void dataDuplicator(hls::stream<ap_uint<DWIDTH> >& inStream,
                    hls::stream<uint32_t>& inSizeStream,
                    hls::stream<ap_uint<PARALLEL_BYTES * 8> >& checksumOutStream,
                    hls::stream<ap_uint<5> >& checksumSizeStream,
                    hls::stream<ap_uint<DWIDTH> >& coreStream,
                    hls::stream<uint32_t>& coreSizeStream) {
    constexpr int c_parallelByte = DWIDTH / 8;
    uint32_t inputSize = inSizeStream.read();

    coreSizeStream << inputSize;

    uint32_t inSize = (inputSize - 1) / c_parallelByte + 1;
    bool inSizeMod = (inputSize % c_parallelByte == 0);

duplicator:
    for (uint32_t i = 0; i < inSize; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<DWIDTH> inValue = inStream.read();
        auto c_size = (i == inSize - 1) && !inSizeMod ? (inputSize % c_parallelByte) : c_parallelByte;
        checksumSizeStream << c_size;
        checksumOutStream << inValue;
        coreStream << inValue;
    }
    checksumSizeStream << 0;
}