void streamDownSizerP2PComp(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                            hls::stream<ap_uint<PACK_WIDTH> >& outStream,
                            hls::stream<uint32_t>& inStreamSize,
                            hls::stream<uint32_t>& outStreamSize,
                            uint32_t no_blocks) {
    const int c_byte_width = 8;
    const int c_input_word = IN_WIDTH / c_byte_width;
    const int c_out_word = PACK_WIDTH / c_byte_width;

    int factor = c_input_word / c_out_word;
    ap_uint<IN_WIDTH> inBuffer = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // input size interms of 512width * 64 bytes after downsizing
        uint32_t sizeOutputV = (size - 1) / c_out_word + 1;

        // Send ouputSize of the module
        outStreamSize << size;

    conv512toV:
        for (int i = 0; i < sizeOutputV; i++) {
#pragma HLS PIPELINE II = 1
            int idx = i % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<PACK_WIDTH> tmpValue = inBuffer.range((idx + 1) * PACK_WIDTH - 1, idx * PACK_WIDTH);
            outStream << tmpValue;
        }
    }
}