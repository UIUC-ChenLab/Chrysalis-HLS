void skipPassCmpBlock(hls::stream<IntVectorStream_dt<8, DBYTES> >& inCmpStream,
                      hls::stream<ap_uint<2> >& rawBlockFlagStream,
                      hls::stream<IntVectorStream_dt<8, DBYTES> >& outCmpStream) {
    IntVectorStream_dt<8, DBYTES> outVal;
stream_skip_cmp_blk:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
    stream_cmp_frm_hdr_blk:
        for (auto i = 0; i < 2; ++i) {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpStream.read(); outVal.strobe > 0; outVal = inCmpStream.read()) {
#pragma HLS PIPELINE II = 1
                if (isRawBlk == false || i == 0) outCmpStream << outVal;
            }
            outVal.strobe = 0;
            if (isRawBlk == false || i == 0) outCmpStream << outVal;
        }
    }
}