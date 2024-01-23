void skipPassRawBlock(hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& inRawStream,
                      hls::stream<ap_uint<2> >& inStbFlagStream,
                      hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& outRawStream,
                      hls::stream<ap_uint<2> >& outStbFlagStream1,
                      hls::stream<ap_uint<2> >& outStbFlagStream2) {
    // read and dump the raw block data when not needed
    ap_uint<IN_DWIDTH + STB_WIDTH> inVal;
    bool stbFlagStrobe = 1;
    ap_uint<2> outFlagVal = 1; // <stb Flag 1-bit><strobe 1-bit>
outer_rbk_loop:
    while (true) {
        inVal = inRawStream.read();
        if (inVal.range(STB_WIDTH - 1, 0) == 0) break;
        auto inFlagVal = inStbFlagStream.read();
        bool isRawBlk = inFlagVal.range(1, 1);
        stbFlagStrobe = inFlagVal.range(0, 0);
        // if last block is present (as control reached here) and strobe for stbFlagStream is 0
        // therefore it is minimum block case, so set isRawBlk flag
        if (stbFlagStrobe == 0) isRawBlk = 1;
        outFlagVal.range(1, 1) = (ap_uint<1>)isRawBlk;
        outStbFlagStream1 << outFlagVal;
        outStbFlagStream2 << outFlagVal;
    skip_pass_raw_block_loop:
        for (; inVal.range(STB_WIDTH - 1, 0) > 0; inVal = inRawStream.read()) {
#pragma HLS PIPELINE II = 1
            if (isRawBlk) outRawStream << inVal;
        }
        // send strobe 0 for end of block
        if (isRawBlk) outRawStream << inVal;
    }
    if (stbFlagStrobe > 0) inStbFlagStream.read();
    // end of data
    inVal = 0;
    outRawStream << inVal;
    outFlagVal = 0;
    outStbFlagStream1 << outFlagVal;
    outStbFlagStream2 << outFlagVal;
}