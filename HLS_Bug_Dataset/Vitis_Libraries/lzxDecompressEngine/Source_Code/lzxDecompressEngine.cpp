void lzxDecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                         hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                         hls::stream<bool>& outStreamEoS,
                         hls::stream<uint64_t>& outSizeStream,
                         const uint32_t _input_size) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    uint32_t input_size1 = _input_size;
    hls::stream<uint16_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint16_t> matchlenStream("matchlenStream");
#pragma HLS STREAM variable = litlenStream depth = 32
#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = offsetStream depth = 32
#pragma HLS STREAM variable = matchlenStream depth = 32

#pragma HLS BIND_STORAGE variable = litlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchlenStream type = FIFO impl = SRL

#pragma HLS dataflow
    lz4MultiByteDecompress<PARALLEL_BYTES>(inStream, litlenStream, litStream, offsetStream, matchlenStream,
                                           input_size1);
    lzMultiByteDecompress<PARALLEL_BYTES, HISTORY_SIZE>(litlenStream, litStream, offsetStream, matchlenStream,
                                                        outStream, outStreamEoS, outSizeStream);
}