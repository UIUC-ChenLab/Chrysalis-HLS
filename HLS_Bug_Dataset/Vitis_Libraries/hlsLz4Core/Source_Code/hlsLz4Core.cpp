void hlsLz4Core(hls::stream<data_t>& inStream,
                hls::stream<data_t>& outStream,
                hls::stream<bool>& outStreamEos,
                hls::stream<uint32_t>& compressedSize,
                uint32_t max_lit_limit[NUM_BLOCK],
                uint32_t input_size,
                uint32_t core_idx) {
    hls::stream<ap_uint<32> > compressdStream("compressdStream");
    hls::stream<ap_uint<32> > bestMatchStream("bestMatchStream");
    hls::stream<ap_uint<32> > boosterStream("boosterStream");
#pragma HLS STREAM variable = compressdStream depth = 8
#pragma HLS STREAM variable = bestMatchStream depth = 8
#pragma HLS STREAM variable = boosterStream depth = 8

#pragma HLS BIND_STORAGE variable = compressdStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = boosterStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::lzCompress<M_LEN, MIN_MAT, LZ_MAX_OFFSET_LIM>(inStream, compressdStream, input_size);
    xf::compression::lzBestMatchFilter<M_LEN, OFFSET_WIN>(compressdStream, bestMatchStream, input_size);
    xf::compression::lzBooster<MAX_M_LEN>(bestMatchStream, boosterStream, input_size);
    xf::compression::lz4Compress<MAX_LIT_CNT, NUM_BLOCK>(boosterStream, outStream, max_lit_limit, input_size,
                                                         outStreamEos, compressedSize, core_idx);
}