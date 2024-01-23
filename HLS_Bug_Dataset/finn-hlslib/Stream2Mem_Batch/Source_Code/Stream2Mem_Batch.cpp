void Stream2Mem_Batch(hls::stream<ap_uint<DataWidth> > & in, ap_uint<DataWidth> * out, const unsigned int numReps) {
  const unsigned int indsPerRep = numBytes / (DataWidth / 8);
  unsigned int rep = 0;
  // make sure Stream2Mem does not get inlined here
  // we lose burst inference otherwise
  while (rep != numReps) {
    unsigned int repsLeft = numReps - rep;
    if ((repsLeft & 0xF) == 0) {
      // repsLeft divisable by 16, write 16 images
      Stream2Mem<DataWidth, numBytes * 16>(in, &out[rep * indsPerRep]);
      rep += 16;
    } else {
      // fallback, write single image
      Stream2Mem<DataWidth, numBytes>(in, &out[rep * indsPerRep]);
      rep += 1;
    }
  }
}

// Content of called function
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