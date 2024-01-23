void Matrix_Vector_Activate_Stream_Batch(hls::stream<TI> &in,
          hls::stream<TO> &out,
          hls::stream<ap_uint<PE*SIMD*TW::width>> &weight,
          TA  const &activation,
          int const  reps,
          R const &r) {

  // how many different rows each neuron will compute
  // alternatively: number of vertical matrix chunks
  unsigned const  NF = MatrixH / PE;

  // how many synapse groups each row is split into
  // alternatively: number of horizontal matrix chunks
  unsigned const  SF = MatrixW / SIMD;

  // input vector buffers
  TI  inputBuf[SF];
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  // accumulators
  decltype(activation.init(0,0))  accu[1][PE];
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0
  // unpacked and packed buffers for weight stream
  Weights_Tile<SIMD, TW, PE > w;
#pragma HLS ARRAY_PARTITION variable=w.m_weights complete dim=0
  ap_uint<PE * SIMD * TW::width> W_packed;


  unsigned  nf   = 0;
  unsigned  sf   = 0;
  unsigned  tile = 0; // invariant: tile = nf*SF + sf
  
  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelinening the way we want
  unsigned const TOTAL_FOLD = NF * SF;
  for(unsigned  i = 0; i < reps * TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
    TI  inElem;

    if(nf == 0) {
      // read input from stream
      inElem = in.read();
      // store in appropriate buffer for reuse
      inputBuf[sf] = inElem;
    }
    else {
      // reuse buffered input
      inElem = inputBuf[sf];
    }

    // read from the parameter stream
    W_packed = weight.read();
    for (unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      w.m_weights[pe] = W_packed((pe+1)*SIMD*TW::width-1,pe*SIMD*TW::width);
    }

    // Threshold Initialisation
    if(sf == 0) {
      for(unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      accu[0][pe] = activation.init(nf, pe);
      }
    }

    // compute matrix-vector product for each processing element
    for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      auto const  act = TSrcI()(inElem, 0);
      auto const  wgt = TWeightI()(w[pe]);
      //auto const  wgt = w[pe];
      accu[0][pe] = mac<SIMD>(accu[0][pe], wgt, act, r, 0);
    }

    // keep track of which folded synapse/neuron we are processing
    ++tile;
    if(++sf == SF) {
      // produce output and clear accumulators
      auto  outElem = TDstI().template operator()<TO>();
      for (unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
      outElem(pe,0,1) = activation.activate(nf, pe, accu[0][pe]);
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

// Content of called function
T mac(T const &a, TC const &c, TD const &d, R const &r) {
#pragma HLS inline
  T  res = a;
  for(unsigned  i = 0; i < N; i++) {
#pragma HLS unroll
    res += mul(c[i], d[i], r);
  }
  return  res;
}