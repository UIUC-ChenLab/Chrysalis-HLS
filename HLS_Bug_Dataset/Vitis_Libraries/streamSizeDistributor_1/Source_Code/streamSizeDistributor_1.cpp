void streamSizeDistributor(hls::stream<uint32_t>& inStream, hls::stream<uint32_t> outStream[SLAVES]) {
    do {
        uint32_t i = inStream.read();
        for (int n = 0; n < SLAVES; n++) {
#pragma HLS UNROLL
            outStream[n] << i;
        }
        if (i == 0) break;
    } while (1);
}