void dataDuplicator(hls::stream<IntVectorStream_dt<8, PARALLEL_BYTES> >& inStream,
                    hls::stream<ap_uint<32> >& checksumInitStream,
                    hls::stream<ap_uint<PARALLEL_BYTES * 8> >& checksumOutStream,
                    hls::stream<ap_uint<5> >& checksumSizeStream,
                    hls::stream<ap_uint<DWIDTH + SIZE_DWIDTH> >& coreStream) {
    constexpr int incr = DWIDTH / 8;

    // Checksum initial data
    if (STRATEGY == 0) {
        checksumInitStream << ~0;
    } else {
        checksumInitStream << 1;
    }

duplicator:
    while (1) {
        IntVectorStream_dt<8, PARALLEL_BYTES> tmpVal = inStream.read();
        // Last will be high if strobe is 0
        bool last = (tmpVal.strobe == 0);
        ap_uint<DWIDTH + SIZE_DWIDTH> outVal = 0;
        // First SIZE_DWIDTH bits will be no. of valid bytes
        outVal.range(SIZE_DWIDTH - 1, 0) = tmpVal.strobe;
        // Checksum requires the parallel valid bytes
        checksumSizeStream << (ap_uint<5>)tmpVal.strobe;
        // Data parallel write
        for (auto i = SIZE_DWIDTH, j = 0; i < (DWIDTH + SIZE_DWIDTH); i += incr) {
#pragma HLS UNROLL
            outVal.range(i + 7, i) = tmpVal.data[j++];
        }
        // Core Data Stream
        coreStream << outVal;
        if (last) break;
        // Checksum Data Stream
        checksumOutStream << outVal.range(DWIDTH + SIZE_DWIDTH - 1, SIZE_DWIDTH);
    }
}