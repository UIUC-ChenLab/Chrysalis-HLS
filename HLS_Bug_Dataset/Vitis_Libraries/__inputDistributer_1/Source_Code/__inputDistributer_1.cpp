void __inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                        hls::stream<ap_uint<68> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 8> inVal;
    ap_uint<68> rawVal;
    ap_uint<68> outVal;
    uint8_t coreIdx = 0;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            // assign values
            rawVal.range(3, 0) = inVal.strobe;
            outVal.range(3, 0) = inVal.strobe;
            for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                rawVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
                outVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
            }
            // write to output streams
            if (!isRawBlock) outStream[coreIdx] << outVal;
            outStrdStream << rawVal;
            dataSize += inVal.strobe;
        }
        // End of block/file
        if (!isRawBlock) outStream[coreIdx] << 0;
        outStrdStream << 0;
        // write meta data
        if (dataSize > 0) {
            // send metadata to packer
            metaVal.strobe = 1;
            metaVal.data[0] = dataSize;
            blockMetaStream << metaVal;
            if (BLOCK_SIZE > (64 * 1024)) {
                metaVal.data[0] = dataSize >> 16;
            } else {
                metaVal.data[0] = 0;
            }
            blockMetaStream << metaVal;
        } else {
            // strobe 0 for end of data, exit condition
            metaVal.strobe = 0;
            blockMetaStream << metaVal;
            break;
        }
        ++coreIdx;
        coreIdx = ((coreIdx < CORE_COUNT) ? coreIdx : 0);
        // coreIdx = (coreIdx + 1) % CORE_COUNT; // increment within limits
    }
// terminate all lz77 blocks
terminate_blocks:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
        if (i != coreIdx) outStream[i] << 0; // "coreIdx" lz77 already terminated
    }
}