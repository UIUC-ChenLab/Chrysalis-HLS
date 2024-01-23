void Stream2Mem(hls::stream<ap_uint<DataWidth> > & in, ap_uint<DataWidth> * out) {
  static_assert(DataWidth % 8 == 0, "");
  const unsigned int numWords = numBytes / (DataWidth / 8);
  static_assert(numWords != 0, "");
  for (unsigned int i = 0; i < numWords; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<DataWidth> e = in.read();
	out[i] = e;
  }
}