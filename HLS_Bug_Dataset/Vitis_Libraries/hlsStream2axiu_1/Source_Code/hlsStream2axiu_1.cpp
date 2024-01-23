void hlsStream2axiu(hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> >& inputStream,
                    hls::stream<ap_axiu<OUT_DWIDTH, TUSR_DWIDTH, 0, 0> >& outAxiStream) {
    constexpr int c_maxByteCnt = OUT_DWIDTH / 8;
    IntVectorStream_dt<8, OUT_DWIDTH / 8> inVal = inputStream.read();
    ap_axiu<OUT_DWIDTH, TUSR_DWIDTH, 0, 0> t1;
    auto strb = inVal.strobe;
    ap_uint<OUT_DWIDTH> tmpVal;
    ap_uint<32> outSize = 0;

    for (auto i = 0, j = 0; i < OUT_DWIDTH; i += c_maxByteCnt) {
#pragma HLS UNROLL
        tmpVal.range(i + 7, i) = inVal.data[j++];
    }
    t1.data = tmpVal;
    ap_uint<OUT_DWIDTH / 8> cntr = 0;
    for (auto i = 0; i < c_maxByteCnt; i++) {
#pragma HLS UNROLL
        cntr.range(i, i) = (i < strb) ? 1 : 0;
    }
    t1.strb = cntr;
    t1.keep = cntr;
    t1.last = 0;
    if (TUSR_DWIDTH != 0) t1.user = 0;
    if (strb == 0) {
        t1.last = 1;
        outAxiStream << t1;
    }

HLS2AXIS:
    while (strb != 0) {
#pragma HLS PIPELINE II = 1
        outSize += strb;
        inVal = inputStream.read();
        strb = inVal.strobe;
        if (strb == 0) {
            t1.last = 1;
            if (TUSR_DWIDTH != 0) t1.user = outSize;
        }
        outAxiStream << t1;
        for (auto i = 0, j = 0; i < OUT_DWIDTH; i += c_maxByteCnt) {
#pragma HLS UNROLL
            tmpVal.range(i + 7, i) = inVal.data[j++];
        }
        for (auto i = 0; i < c_maxByteCnt; i++) {
#pragma HLS UNROLL
            cntr.range(i, i) = (i < strb) ? 1 : 0;
        }
        t1.data = tmpVal;
        t1.strb = cntr;
        t1.keep = cntr;
        t1.last = 0;
    }
}