void inflateMultiCores(hls::stream<ap_axiu<64, 0, 0, 0> >& inaxistream,
                       hls::stream<ap_axiu<64, 0, 0, 0> >& outaxistream) {
    constexpr int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<72> > axi2HlsStrm[NUM_CORE];
    hls::stream<ap_uint<72> > inflateOut[NUM_CORE];
    hls::stream<ap_uint<16> > hlsDownStrm[NUM_CORE];
    hls::stream<bool> hlsEos[NUM_CORE];

#pragma HLS STREAM variable = axi2HlsStrm depth = 4096
#pragma HLS STREAM variable = inflateOut depth = 4096
#pragma HLS STREAM variable = hlsDownStrm depth = 32
#pragma HLS STREAM variable = hlsEos depth = 32

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = inflateOut type = fifo impl = URAM

#pragma HLS dataflow disable_start_propagation
    details::axi2HlsDistributor<NUM_CORE>(inaxistream, axi2HlsStrm);

    for (auto i = 0; i < NUM_CORE; i++) {
#pragma HLS UNROLL
        details::bufferDownsizer<NUM_CORE, PARALLEL_BYTES>(axi2HlsStrm[i], hlsDownStrm[i], hlsEos[i]);
        details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
            hlsDownStrm[i], hlsEos[i], inflateOut[i]);
    }

    details::hls2AxiMerger<c_parallelBit, NUM_CORE>(outaxistream, inflateOut);
}

// Content of called function
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

// Content of called function
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