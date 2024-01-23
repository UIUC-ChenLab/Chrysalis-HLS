void streamCmpStrdFrame(hls::stream<ap_uint<68> >& inRawBStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& inCmpBStream,
                        hls::stream<ap_uint<2> >& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    IntVectorStream_dt<8, 8> outVal;
    ap_uint<24> strdBlockHeader = 1; // bit-0 = 1, indicating last block, bits 1-2 = 0, indicating raw block
stream_cmp_file:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
        // read frame content size
        outVal = inCmpBStream.read();
        strdBlockHeader.range(10, 3) = (uint8_t)outVal.data[0];
        strdBlockHeader.range(18, 11) = (uint8_t)outVal.data[1];
        strdBlockHeader.range(23, 19) = (uint8_t)outVal.data[2];
    // write the frame header, written for each block of input data as stated in its meta data
    // unsigned t = 0;
    write_frame_header:
        for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        if (isRawBlk) {
            // Write stored block header
            outVal.data[0] = strdBlockHeader.range(7, 0);
            outVal.data[1] = strdBlockHeader.range(15, 8);
            outVal.data[2] = strdBlockHeader.range(23, 16);
            outVal.strobe = 3;
            outStream << outVal;
        write_raw_blk_data:
            for (auto rbVal = inRawBStream.read(); rbVal.range(3, 0) > 0; rbVal = inRawBStream.read()) {
#pragma HLS PIPELINE II = 1
                for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                    outVal.data[i] = rbVal.range((i * 8) + 11, (i * 8) + 4);
                }
                outVal.strobe = rbVal.range(3, 0);
                outStream << outVal;
            }
        } else {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << outVal;
            }
        }
    }
    // dump last strobe 0
    inRawBStream.read();
    // end of file
    outVal.strobe = 0;
    outStream << outVal;
}