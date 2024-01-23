void Mem2Stream_Batch_external_wmem(ap_uint<DataWidth> * in,
        hls::stream<ap_uint<DataWidth> > & out, const unsigned int numReps) {
    unsigned int rep = 0;
    while (rep != numReps) {
        Mem2Stream<DataWidth, numBytes>(&in[0], out);
        rep += 1;
    }
}

// Content of called function
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