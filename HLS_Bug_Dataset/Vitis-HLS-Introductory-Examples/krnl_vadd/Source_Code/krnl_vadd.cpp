void krnl_vadd(uint32_t* in1, uint32_t* in2, uint32_t* out, int vSize) {
    static hls::stream<uint32_t> inStream1("input_stream_1");
    static hls::stream<uint32_t> inStream2("input_stream_2");
    static hls::stream<uint32_t> outStream("output_stream");
#pragma HLS INTERFACE m_axi port = in1 bundle = gmem0 depth = 4096
#pragma HLS INTERFACE m_axi port = in2 bundle = gmem1 depth = 4096
#pragma HLS INTERFACE m_axi port = out bundle = gmem0 depth = 4096

#pragma HLS dataflow
    // dataflow pragma instruct compiler to run following three APIs in parallel
    read_input(in1, inStream1, vSize);
    read_input(in2, inStream2, vSize);
    compute_add(inStream1, inStream2, outStream, vSize);
    write_result(out, outStream, vSize);
}