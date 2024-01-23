void LabelSelect_Batch(hls::stream<ap_uint<PECount * In_T::width> > & in,
        hls::stream<Out_T> & out, const unsigned int numReps) {

  // Check that classes, aka. labels / indeces, can be encoded as non-negative outputs
  static_assert(clog2(NumClasses) <= Out_T::width - Out_T::sign_flag, "");
  static In_T const  In_T_MIN_VAL = (In_T(-1)<0)? 1<<(In_T::width-1) : 0;

  // Array of encountered top values
  //  - maintains topval[i] <= topval[i+1]
  //  - keeps in alignment with toplabels
  In_T topval[NumTop];
#pragma HLS ARRAY_PARTITION variable=topval complete dim=1
  Out_T toplabels[NumTop];
#pragma HLS ARRAY_PARTITION variable=toplabels complete dim=1

  for(unsigned int reps=0; reps<numReps; reps++){
    unsigned int idx = 0;
    for(unsigned int topx=0; topx<NumTop; topx++){
#pragma HLS UNROLL
      topval   [topx] = In_T_MIN_VAL;
      toplabels[topx] = 0;
    }
    for(unsigned int block=0; block<(NumClasses/PECount); block++){
#pragma HLS pipeline style=flp II=1
      ap_uint<PECount * In_T::width> const  inval = in.read();
      for(unsigned int elem=0; elem<PECount; elem++){
#pragma HLS UNROLL
        // Extract individual input
        unsigned const  lowBit = elem * In_T::width;
        unsigned const  highBit = (elem+1) * In_T::width - 1;
        In_T const  val = inval(highBit,lowBit);

        // Compare input against all current tops
        bool  cmp[NumTop+1];
        for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
          cmp[i] = val > topval[i];
        }
        cmp[NumTop] = false;

        // Shift input into top array at the highest index where it is greater
        for(unsigned  i = 0; i < NumTop; i++) {
#pragma HLS UNROLL
          if(cmp[i]) {
            if(cmp[i+1]) {
              // Shift
              topval   [i] = topval   [i+1];
              toplabels[i] = toplabels[i+1];
            }
            else {
              // Insert
              topval   [i] = val;
              toplabels[i] = idx;
            }
          }
        }
        idx++;
      }
    }

    // Output - index of highest value first
    for(unsigned int topx = 0; topx < NumTop; topx++){
      out.write(toplabels[NumTop - topx - 1]);
    }
  }
}