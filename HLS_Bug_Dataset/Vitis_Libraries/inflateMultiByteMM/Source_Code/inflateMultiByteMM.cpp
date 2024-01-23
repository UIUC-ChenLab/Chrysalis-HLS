void inflateMultiByteMM(const ap_uint<GMEM_DATAWIDTH>* in,
                        ap_uint<GMEM_DATAWIDTH>* out,
                        uint32_t* encodedSize,
                        uint32_t inputSize) {
#pragma HLS dataflow
    constexpr int c_inBitWidth = 16;
    constexpr int c_burstDepth = 2 * GMEM_BRST_SIZE;

    // Internal Streams
    hls::stream<ap_uint<GMEM_DATAWIDTH> > mm2sStream;
    hls::stream<ap_uint<c_inBitWidth> > inInfateStream;
    hls::stream<bool> inInfateStreamEos;
    hls::stream<uint32_t> sizeStream;
    hls::stream<uint32_t> sizeStreamV;
    hls::stream<ap_uint<GMEM_DATAWIDTH + PARALLEL_BYTES> > outStream;

    // Initialize Size Stream
    uint32_t tmp = inputSize;
    sizeStreamV.write(tmp);

#pragma HLS STREAM variable = mm2sStream depth = c_burstDepth
#pragma HLS STREAM variable = inInfateStream depth = c_burstDepth
#pragma HLS STREAM variable = inInfateStreamEos depth = c_burstDepth
#pragma HLS STREAM variable = sizeStream depth = 4
#pragma HLS STREAM variable = sizeStreamV depth = 4
#pragma HLS STREAM variable = outStream depth = c_burstDepth

    // MM2S
    xf::compression::details::mm2sSimple<GMEM_DATAWIDTH, GMEM_BRST_SIZE>(in, mm2sStream, sizeStream, sizeStreamV);

    // Downsizer
    xf::compression::details::streamDownsizerEos<GMEM_DATAWIDTH, c_inBitWidth>(mm2sStream, sizeStream, inInfateStream,
                                                                               inInfateStreamEos);

    // Decompression
    xf::compression::details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
        inInfateStream, inInfateStreamEos, outStream);

    // S2MM
    xf::compression::details::s2mmAxi<GMEM_DATAWIDTH, GMEM_BRST_SIZE, PARALLEL_BYTES>(out, outStream, encodedSize);
}