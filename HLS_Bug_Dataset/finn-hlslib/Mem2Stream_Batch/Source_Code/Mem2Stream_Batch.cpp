void Mem2Stream_Batch(ap_uint<DataWidth> * in, hls::stream<ap_uint<DataWidth> > & out, const unsigned int numReps) {
  const unsigned int indsPerRep = numBytes / (DataWidth / 8);
  unsigned int rep = 0;
  // make sure Mem2Stream does not get inlined here
  // we lose burst inference otherwise
  while (rep != numReps) {
    unsigned int repsLeft = numReps - rep;
    if ((repsLeft & 0xF) == 0) {
      // repsLeft divisable by 16, read 16 images
      Mem2Stream<DataWidth, numBytes * 16>(&in[rep * indsPerRep], out);
      rep += 16;
    } else {
      // fallback, read single image
      Mem2Stream<DataWidth, numBytes>(&in[rep * indsPerRep], out);
      rep += 1;
    }
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