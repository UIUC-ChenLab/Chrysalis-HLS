void Vector_Vector_Activate_Batch(hls::stream<TI> &in,
				  hls::stream<TO> &out,
				  TW  const &weights,
				  TA  const &activation,
				  int const  reps,
				  R const &r) {

  static_assert(SIMD == 1, "SIMD parallelism not yet supported.");

  // how many different rows each neuron will compute
  // alternatively: number of vertical matrix chunks
  unsigned const  NF = Channels / PE;

  // how many synapse groups each row is split into
  // alternatively: number of horizontal matrix chunks
  // always equal to # kernel pixels since no SIMD
  unsigned const  SF = Kernel_2;
  decltype(activation.init(0,0))  accu[MMV][PE];
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0

  unsigned  nf   = 0;
  unsigned  sf   = 0;
  unsigned  tile = 0; // invariant: tile = nf*SF + sf
  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelinening the way we want
  unsigned const TOTAL_FOLD = NF * SF ;//* Channels/SIMD;
  for(unsigned  i = 0; i < reps * TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
    TI  inElem;
    inElem = in.read();
    // Threshold Initialisation
    if(sf == 0) {
      for(unsigned  pe = 0; pe < PE; pe++) {
        for(unsigned mmv = 0; mmv < MMV; mmv++) {
#pragma HLS UNROLL
          accu[mmv][pe] = activation.init(nf, pe);
        }
      }
    }

    // compute matrix-vector product for each processing element
    auto const &w = weights.weights(tile);
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      auto const  wgt = TWeightI()(w[pe]);
      for (unsigned mmv = 0; mmv < MMV; mmv++){
        auto const  act = TSrcI()(inElem, mmv);
		accu[mmv][pe] += mul(wgt[0], act(pe,mmv), r);
      }
    }

    // keep track of which folded synapse/neuron we are processing
    ++tile;
    if(++sf == SF) {
      // produce output and clear accumulators
      auto  outElem = TDstI().template operator()<TO>();
      for (unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
        for (unsigned mmv = 0; mmv < MMV; mmv++){
#pragma HLS UNROLL
          outElem(pe,mmv,1) = activation.activate(nf, pe, accu[mmv][pe]);
        }
      }
      out.write(outElem);
      // next folded neuron or image
      sf = 0;
      if(++nf == NF) {
	    nf   = 0;
	    tile = 0;
      }
    }
  }
}