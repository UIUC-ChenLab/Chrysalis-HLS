void hls2AxiMerger(hls::stream<ap_axiu<64, 0, 0, 0> >& outKStream, hls::stream<ap_uint<72> > outDataStream[NUM_CORE]) {
    for (auto i = 0; i < NUM_CORE; i++) {
        ap_uint<8> strb = 0;
        ap_uint<64> data;
        ap_uint<72> tmp;
        ap_axiu<64, 0, 0, 0> t1;

        tmp = outDataStream[i].read();

        strb = tmp.range(7, 0);
        t1.data = tmp.range(71, 8);
        t1.strb = strb;
        t1.keep = strb;
        t1.last = 0;
        if (strb == 0) {
            t1.last = 1;
            outKStream << t1;
        }
        while (strb != 0) {
#pragma HLS PIPELINE II = 1
            tmp = outDataStream[i].read();

            strb = tmp.range(7, 0);
            if (strb == 0) {
                t1.last = 1;
            }
            outKStream << t1;

            t1.data = tmp.range(71, 8);
            t1.strb = strb;
            t1.keep = strb;
            t1.last = 0;
        }
    }
}