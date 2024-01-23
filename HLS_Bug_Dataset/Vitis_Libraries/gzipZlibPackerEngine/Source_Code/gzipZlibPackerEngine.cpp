void gzipZlibPackerEngine(hls::stream<ap_uint<68> > inStream[NUM_BLOCKS],
                          hls::stream<ap_uint<68> >& packedStream,
                          hls::stream<ap_uint<64> >& strdStream,
                          hls::stream<ap_uint<16> >& strdSizeStream,
                          hls::stream<uint32_t>& fileSizeStream,
                          hls::stream<ap_uint<32> >& checksumStream,
                          hls::stream<ap_uint<4> >& coreIdxStream,
                          hls::stream<bool>& blckEosStream) {
    ap_uint<68> tmpVal = 0;
    ap_uint<4> core = coreIdxStream.read();

    // Header Handling
    if (STRATEGY == 0) { // GZIP
        tmpVal.range(67, 0) = 0x0000000008088B1F8;
        packedStream << tmpVal;
        tmpVal.range(67, 0) = 0x007803004;
        packedStream << tmpVal;
    } else { // ZLIB
        tmpVal.range(67, 0) = 0x01782;
        packedStream << tmpVal;
    }

// Compressed Data Handling
blckHandler:
    for (bool blckEos = blckEosStream.read(); blckEos == false; blckEos = blckEosStream.read()) {
        core %= NUM_BLOCKS;
        bool blockDone = false;
        for (; blockDone == false;) {
#pragma HLS PIPELINE II = 1
            ap_uint<68> inVal = inStream[core].read();
            blockDone = (inVal.range(3, 0) == 0);
            if (blockDone) break;
            packedStream << inVal;
        }
        core++;
    }

    // Stored Block Header
    ap_uint<16> sizeVal = strdSizeStream.read();
    bool strdFlg = (sizeVal != 0);
    if (strdFlg) {
        tmpVal = ~sizeVal;
        tmpVal <<= 16;
        tmpVal.range(15, 0) = sizeVal;
        tmpVal <<= 12;
        tmpVal.range(11, 0) = 5;
        packedStream << tmpVal;
    }

// Stored Block
strdBlck:
    for (uint16_t size = 0; size < sizeVal; size += 8) {
#pragma HLS PIPELINE II = 1
        uint8_t rSize = 8;
        if (rSize + size > sizeVal) rSize = sizeVal - size;
        tmpVal.range(3, 0) = rSize;
        tmpVal.range(67, 4) = strdStream.read();
        packedStream << tmpVal;
    }

    // Checksum Data
    // Last Block Data
    tmpVal.range(67, 0) = 0xffff0000015;
    packedStream << tmpVal;

    ap_uint<32> tmpChecksum = checksumStream.read();
    if (STRATEGY == 0) {
        tmpVal.range(36, 4) = tmpChecksum;
    } else {
        for (auto i = 0; i < 4; i++) {
#pragma HLS UNROLL
            tmpVal.range((((4 - i) * 8) - 1) + 4, ((3 - i) * 8) + 4) = tmpChecksum.range(((i + 1) * 8) - 1, i * 8);
        }
    }
    tmpVal.range(3, 0) = 4;
    packedStream << tmpVal;
    // Input Size Data
    if (STRATEGY == 0) {
        tmpVal.range(67, 4) = fileSizeStream.read();
        tmpVal.range(3, 0) = 4;
        packedStream << tmpVal;
    }
    tmpVal = 0;
    packedStream << tmpVal;

    for (auto i = 0; i < NUM_BLOCKS; i++) inStream[i].read();
}