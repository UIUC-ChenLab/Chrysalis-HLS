void __inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 1> inVal;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            if (!isRawBlock) outStream << inVal;
            outStrdStream << inVal;
            ++dataSize;
        }
        // End of block/file
        if (!isRawBlock) outStream << inVal;
        outStrdStream << inVal;
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
    }
}