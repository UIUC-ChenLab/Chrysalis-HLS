void Pool_batch(hls::stream<TI> &in,
                  hls::stream<TO> &out,
                  TA  const &function,
                  int const  reps) {

  constexpr unsigned  NF = Channels / PE;
  constexpr unsigned  SF = TotalK;
  constexpr unsigned  TOTAL_FOLD = NF * SF ;

  decltype(function.init())  accu[PE];
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0

  unsigned  sf   = 0;
  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelining the way we want
  for(unsigned  i = 0; i < reps * TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
    TI  pixel_slice;
    pixel_slice = in.read();

    // Threshold Initialisation
    if(sf == 0) {
      for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
        accu[pe] = function.init();
      }
    }

    auto const  slice_channels = TSrcI()(pixel_slice,0);
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
        accu[pe] = function.pool(slice_channels(pe,0), accu[pe]);
    }

    // keep track of which folded synapse/neuron we are processing
    if(++sf == SF) {
      // produce output and clear accumulators
      auto  outElem = TDstI().template operator()<TO>();
      for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
          outElem(pe,0,1) = function.activate(accu[pe]); //
      }
      out.write(outElem);
      // next folded neuron or image
      sf = 0;
    }
  }
}