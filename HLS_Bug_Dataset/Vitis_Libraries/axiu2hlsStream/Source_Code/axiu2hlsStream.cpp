void axiu2hlsStream(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inAxiStream,
                    hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> >& outHlsStream) {
    constexpr int c_maxByteCnt = IN_DWIDTH / 8;
    IntVectorStream_dt<8, c_maxByteCnt> val;
    ap_uint<IN_DWIDTH / 8> cntr = 0;
    // Read AXI Stream
    ap_axiu<IN_DWIDTH, 0, 0, 0> inData;
    inData = inAxiStream.read();
    bool last = inData.last;
    ap_uint<IN_DWIDTH> tmpVal = inData.data;
    ap_uint<IN_DWIDTH / 8> keep = inData.keep;
    val.strobe = c_maxByteCnt;

axi2Hls:
    while (!last) {
#pragma HLS PIPELINE
        // Write the data in an aggregated manner in internal hls stream
        for (auto i = 0, j = 0; i < IN_DWIDTH; i += c_maxByteCnt) {
#pragma HLS UNROLL
            val.data[j++] = tmpVal.range(i + 7, i);
        }
        outHlsStream << val;

        inData = inAxiStream.read();
        last = inData.last;
        tmpVal = inData.data;
        keep = inData.keep;
    }

    // Write the data in an aggregated manner in internal hls stream
    for (auto i = 0, j = 0; i < IN_DWIDTH; i += c_maxByteCnt) {
#pragma HLS UNROLL
        val.data[j++] = tmpVal.range(i + 7, i);
    }
    while (keep) {
        cntr += (keep & 0x1);
        keep >>= 1;
    }
    val.strobe = cntr;
    outHlsStream << val;
    val.strobe = 0;
    outHlsStream << val;
}