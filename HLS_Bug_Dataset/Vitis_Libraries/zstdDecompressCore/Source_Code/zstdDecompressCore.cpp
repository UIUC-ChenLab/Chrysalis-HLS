void zstdDecompressCore(hls::stream<ap_axiu<IN_BYTES * 8, 0, 0, 0> >& inStream,
                        hls::stream<ap_axiu<OUT_BYTES * 8, 0, 0, 0> >& outStream) {
    // Core function to use zstdDecompressStream module with zlib data movers
    // const values
    const uint16_t c_inStreamDepth = 32; // 32 for INPUT_BYTE=4 and 16 for INPUT_BYTE=8
    const uint8_t c_minStreamDepth = 4;
    // internal streams
    hls::stream<ap_uint<IN_BYTES * 8> > zstdCoreInStream("zstdCoreInStream");
    hls::stream<ap_uint<(OUT_BYTES * 8) + OUT_BYTES> > outputStream("outputStream");
    hls::stream<ap_uint<4> > inStrobe("inStrobe");

#pragma HLS STREAM variable = zstdCoreInStream depth = c_inStreamDepth
#pragma HLS STREAM variable = outputStream depth = c_inStreamDepth
#pragma HLS STREAM variable = inStrobe depth = c_inStreamDepth

#pragma HLS dataflow

    details::kStreamReadZstdDecomp<IN_BYTES>(inStream, zstdCoreInStream, inStrobe);

    zstdDecompressStream<IN_BYTES, OUT_BYTES, BLOCK_SIZE_KB, LZ_MAX_OFFSET, LMO_WIDTH, SEQ_LOW_LATENCY, LOW_LATENCY>(
        zstdCoreInStream, inStrobe, outputStream);

    details::kStreamWriteZstdDecomp<OUT_BYTES * 8>(outStream, outputStream);
}

// Content of called function
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