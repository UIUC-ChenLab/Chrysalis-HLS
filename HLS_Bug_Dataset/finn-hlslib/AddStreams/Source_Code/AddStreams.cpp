void AddStreams(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                hls::stream<ap_uint<NumChannels * Out_t::width>> &out) {

  for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<NumChannels * In1_t::width> e1 = in1.read();
    ap_uint<NumChannels * In2_t::width> e2 = in2.read();
    ap_uint<NumChannels * Out_t::width> e;
    for (unsigned int j = 0; j < NumChannels; j++) {
#pragma HLS UNROLL
      In1_t op1 = e1((j + 1) * In1_t::width - 1, j * In1_t::width);
      In2_t op2 = e2((j + 1) * In2_t::width - 1, j * In2_t::width);
      Out_t sum = op1 + op2 + offset;
      e((j + 1) * Out_t::width - 1, j * Out_t::width) = sum;
    }
    out.write(e);
  }
}