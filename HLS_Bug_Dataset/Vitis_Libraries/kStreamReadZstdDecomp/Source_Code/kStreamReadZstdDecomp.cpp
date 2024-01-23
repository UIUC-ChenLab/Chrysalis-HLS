void kStreamReadZstdDecomp(hls::stream<ap_axiu<8 * PARALLEL_BYTE, 0, 0, 0> >& inKStream,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& zstdCoreInStream,
                           hls::stream<ap_uint<4> >& inStrobe) {
    // write input data to core module from kernel axi stream
    bool last = true;
    do {
#pragma HLS PIPELINE II = 1
        ap_axiu<8 * PARALLEL_BYTE, 0, 0, 0> tmp = inKStream.read();
        zstdCoreInStream << tmp.data;
        last = tmp.last;
        inStrobe << tmp.strb;
    } while (last == false);
    zstdCoreInStream << 0;
    inStrobe << 0; // Terminate condition
}