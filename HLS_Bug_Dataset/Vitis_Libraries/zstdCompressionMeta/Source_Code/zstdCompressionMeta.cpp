void zstdCompressionMeta(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                         hls::stream<ap_uint<2> >& strdBlockFlagStream,
                         hls::stream<DT>& outMetaStream) {
    uint8_t i = 0;
    DT litCnt = 0;
    ap_uint<2> stbFVal = 1; // <stb Flag 1-bit><strobe 1-bit>
gen_meta_loop:
    for (auto inMetaVal = metaStream.read(); inMetaVal.strobe > 0; inMetaVal = metaStream.read()) {
#pragma HLS PIPELINE off
        litCnt = inMetaVal.data[0];
        outMetaStream << litCnt;
        if (i == 0) {
            stbFVal.range(1, 1) = (ap_int<1>)(litCnt > BLOCK_SIZE - 2048);
            strdBlockFlagStream << stbFVal;
        }
        i = (i + 1) & 1;
    }
    // end of all data
    stbFVal = 0;
    strdBlockFlagStream << stbFVal;
}