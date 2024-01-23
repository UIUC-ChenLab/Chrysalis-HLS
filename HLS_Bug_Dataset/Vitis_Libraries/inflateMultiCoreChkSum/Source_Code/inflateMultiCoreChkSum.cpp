void inflateMultiCoreChkSum(hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& inaxistream,
                            hls::stream<ap_axiu<PARALLEL_BYTES * 8, TUSER_WIDTH, 0, 0> >& outaxistream) {
    const int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > axi2HlsStrm[NUM_CORE];
    hls::stream<ap_uint<16> > hlsDownStream[NUM_CORE];
    hls::stream<bool> hlsEos[NUM_CORE];
    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > inflateStrm[NUM_CORE];
    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > inflateDist[2];
    hls::stream<ap_uint<32> > chkSum[NUM_CORE + 1];
    // hls::stream<bool> errorStrm;
    hls::stream<ap_uint<(PARALLEL_BYTES * 8)> > lzData("lzData");
    hls::stream<ap_uint<5> > lzStrb("lzStrb");

#pragma HLS STREAM variable = axi2HlsStrm depth = 1024
#pragma HLS STREAM variable = hlsDownStream depth = 32
#pragma HLS STREAM variable = hlsEos depth = 32
#pragma HLS STREAM variable = inflateStrm depth = 1024
#pragma HLS STREAM variable = inflateDist depth = 8
#pragma HLS STREAM variable = chkSum depth = 8
// #pragma HLS STREAM variable = errorStrm depth = 8
#pragma HLS STREAM variable = lzData depth = 8
#pragma HLS STREAM variable = lzStrb depth = 8

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = hlsDownStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = hlsEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inflateStrm type = fifo impl = BRAM
#pragma HLS BIND_STORAGE variable = inflateDist type = fifo impl = SRL

#pragma HLS dataflow disable_start_propagation
    details::kStreamDistributor<NUM_CORE, c_parallelBit>(inaxistream, axi2HlsStrm);

    for (uint8_t i = 0; i < NUM_CORE; i++) {
#pragma HLS UNROLL
        details::simpleDownSizer<PARALLEL_BYTES, c_parallelBit, 16>(axi2HlsStrm[i], hlsDownStream[i], hlsEos[i]);
        details::zlibWithAdler<DECODER, PARALLEL_BYTES, FILE_FORMAT, HISTORY_SIZE>(hlsDownStream[i], hlsEos[i],
                                                                                   inflateStrm[i], chkSum[i]);
    }

    xf::compression::details::streamMultiDistributor<PARALLEL_BYTES, NUM_CORE>(inflateStrm, inflateDist);
    xf::compression::details::dataStrbSplitter<PARALLEL_BYTES, NUM_CORE>(inflateDist[0], lzData, lzStrb);
    xf::compression::details::adler32<PARALLEL_BYTES, NUM_CORE>(lzData, lzStrb, chkSum[NUM_CORE]);
    //    xf::compression::details::chckSumComparatorMulti<NUM_CORE>(chkSum, chkSum[NUM_CORE], errorStrm);
    details::hls2AXIWithTUSERMerger<c_parallelBit, TUSER_WIDTH, NUM_CORE>(outaxistream, inflateDist[1], chkSum,
                                                                          chkSum[NUM_CORE]);
}

// Content of called function
void adler32(hls::stream<ap_uint<W * 8> >& inStrm,
             hls::stream<ap_uint<5> >& inPackLenStrm,
             hls::stream<ap_uint<32> >& outStrm) {
    // To be deprecated
    hls::stream<bool> endInPackLenStrm;
    hls::stream<bool> endOutStrm;
    hls::stream<ap_uint<32> > adlerStrm;
#pragma HLS STREAM variable = endInPackLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4
#pragma HLS STREAM variable = adlerStrm depth = 4

    for (int i = 0; i < NUMCORES; i++) {
        endInPackLenStrm << false;
        endInPackLenStrm << true;
        adlerStrm << 1;
        xf::security::adler32<W>(adlerStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
        endOutStrm.read();
        endOutStrm.read();
    }
}