void Mem2Stream(ap_uint<DataWidth> * in, hls::stream<ap_uint<DataWidth> > & out) {
  static_assert(DataWidth % 8 == 0, "");
  const unsigned int numWords = numBytes / (DataWidth / 8);
  static_assert(numWords != 0, "");
  for (unsigned int i = 0; i < numWords; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<DataWidth> e = in[i];
    out.write(e);
  }
}