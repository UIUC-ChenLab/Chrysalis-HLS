void axi2HlsDistributor(hls::stream<ap_axiu<64, 0, 0, 0> >& in, hls::stream<ap_uint<72> > out[NUM_CORE]) {
    for (auto i = 0; i < NUM_CORE; i++) {
        bool last = false;
        ap_axiu<64, 0, 0, 0> tmp;
        ap_uint<8> kp = 0;
        ap_uint<8> cntr = 0;
        ap_uint<72> tmpVal = 0;
        while (last == false) {
#pragma HLS PIPELINE II = 1
            tmp = in.read();
            tmpVal.range(71, 8) = tmp.data;
            kp = tmp.keep;
            tmpVal.range(7, 0) = tmp.keep;
            if (kp == 0xFF)
                tmpVal.range(7, 0) = 8;
            else
                break;
            out[i] << tmpVal;
            last = tmp.last;
        }
        if (kp != 0xFF) {
            while (kp) {
                cntr += (kp & 0x1);
                kp >>= 1;
            }
            tmpVal.range(7, 0) = cntr;
            out[i] << tmpVal;
        }
        out[i] << 0; // Terminate condition
    }
}