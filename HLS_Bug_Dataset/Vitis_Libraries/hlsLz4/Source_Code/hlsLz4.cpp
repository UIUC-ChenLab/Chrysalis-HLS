void hlsLz4(const data_t* in,
            data_t* out,
            const uint32_t input_idx[NUM_BLOCK],
            const uint32_t output_idx[NUM_BLOCK],
            const uint32_t input_size[NUM_BLOCK],
            uint32_t output_size[NUM_BLOCK],
            uint32_t max_lit_limit[NUM_BLOCK]) {
    hls::stream<ap_uint<8> > inStream[NUM_BLOCK];
    hls::stream<bool> outStreamEos[NUM_BLOCK];
    hls::stream<ap_uint<8> > outStream[NUM_BLOCK];
#pragma HLS STREAM variable = outStreamEos depth = 2
#pragma HLS STREAM variable = inStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = outStream depth = c_gmemBurstSize

#pragma HLS BIND_STORAGE variable = outStreamEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL

    hls::stream<uint32_t> compressedSize[NUM_BLOCK];

#pragma HLS dataflow
    xf::compression::details::mm2multStreamSize<8, NUM_BLOCK, DATAWIDTH, BURST_SIZE>(in, input_idx, inStream,
                                                                                     input_size);

    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        // lz4Core is instantiated based on the NUM_BLOCK
        hlsLz4Core<ap_uint<8>, DATAWIDTH, BURST_SIZE, NUM_BLOCK>(inStream[i], outStream[i], outStreamEos[i],
                                                                 compressedSize[i], max_lit_limit, input_size[i], i);
    }

    xf::compression::details::multStream2MM<8, NUM_BLOCK, DATAWIDTH, BURST_SIZE>(
        outStream, outStreamEos, compressedSize, output_idx, out, output_size);
}

// Content of called function
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