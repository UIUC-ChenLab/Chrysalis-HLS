void streamK2Dm(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& out,
                hls::stream<bool>& outEoS,
                hls::stream<SIZE_DT>& dataSize,
                hls::stream<ap_axiu<PARALLEL_BYTES * 8, TUSR_DWIDTH, 0, 0> >& dmInStream,
                SIZE_DT numItr,
                SIZE_DT inputSize,
                SIZE_DT blckSize) {
    SIZE_DT blckNum = (inputSize - 1) / blckSize + 1;
    for (auto z = 0; z < numItr * blckNum; z++) {
        SIZE_DT cntr = 0;
        ap_axiu<PARALLEL_BYTES * 8, TUSR_DWIDTH, 0, 0> inValue;
        auto keep = 0;
        bool last = false;

        while (!last) {
#pragma HLS PIPELINE II = 1
            inValue = dmInStream.read();
            outEoS << 0;
            out << inValue.data;
            last = inValue.last;
            keep = inValue.keep;
            if (last) break;
            cntr += PARALLEL_BYTES;
        }

        auto incr = 0;
        while (keep) {
            incr += (keep & 0x1);
            keep >>= 1;
        }

        cntr += incr;
        dataSize << cntr;
        if (TUSR_DWIDTH != 0) dataSize << inValue.user;
        outEoS << 1;
        out << 0;
    }
}