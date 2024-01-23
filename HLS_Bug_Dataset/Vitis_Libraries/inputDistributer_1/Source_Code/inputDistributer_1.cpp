void inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                      hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                      hls::stream<ap_uint<68> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    constexpr uint16_t c_idataStreamDepth = 256 / CORE_COUNT;
    // Internal streams
    hls::stream<IntVectorStream_dt<8, 8> > intmDataStream("intmDataStream");
    hls::stream<bool> rawBlockFlagStream("rawBlockFlagStream");

#pragma HLS STREAM variable = intmDataStream depth = c_idataStreamDepth
#pragma HLS STREAM variable = rawBlockFlagStream depth = 32

#pragma HLS dataflow
    xf::compression::details::inputBufferMinBlock<BLOCK_SIZE, MIN_BLK_SIZE>(inStream, rawBlockFlagStream,
                                                                            intmDataStream);

    xf::compression::details::__inputDistributer<BLOCK_SIZE, CORE_COUNT>(intmDataStream, rawBlockFlagStream, outStream,
                                                                         outStrdStream, blockMetaStream);
}

// Content of called function
void inputBufferMinBlock(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                         hls::stream<bool>& rawBlockFlagStream,
                         hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    // write data and indicate if it should be raw block or not
    bool not_done = true;
    bool rawFlagNotSent = true;
    IntVectorStream_dt<8, 8> inVal;
stream_data:
    while (not_done) {
        // read data size in bytes
        uint32_t dataSize = 0;
        inVal.strobe = 8;
        rawFlagNotSent = true;
    send_in_block:
        while (inVal.strobe > 0 && dataSize < BLOCK_SIZE) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            if (inVal.strobe > 0) {
                outStream << inVal;
                dataSize += inVal.strobe;
                // indicate if more data than minimum block size
                if (dataSize > MIN_BLK_SIZE && rawFlagNotSent) {
                    rawBlockFlagStream << false;
                    rawFlagNotSent = false;
                }
            }
        }
        if (dataSize > 0 && dataSize < 1 + MIN_BLK_SIZE) rawBlockFlagStream << true;
        // end of block for last block with data
        if (dataSize > 0 && inVal.strobe == 0) outStream << inVal;
        // end of block/file
        inVal.strobe = 0;
        outStream << inVal;
        // terminate condition
        not_done = (dataSize == BLOCK_SIZE);
    }
    // for end of files, value must be false
    rawBlockFlagStream << false;
}

// Content of called function
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