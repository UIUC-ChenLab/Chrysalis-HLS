void Thresholding_Stream_Batch(hls::stream<TI> &in,
                        hls::stream<TO> &out,
                        hls::stream<ap_uint<PE*NumSteps*TT::width>> &weight,
                        int const reps)
{

  // how many different rows each neuron will compute
  // alternatively: number of vertical matrix chunks
  unsigned const NF = NumChannels / PE;

  ThresholdsActivation<1, PE, NumSteps, TT, TO, ActVal, comp::less_equal<TT, TT>> internal_thr;
#pragma HLS ARRAY_PARTITION variable=internal_thr.m_thresholds complete dim=0

  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelinening the way we want
  for (unsigned i = 0; i < reps * ImgDim * NF; i++)
  {
#pragma HLS pipeline style=flp II=1

    ap_uint<PE*NumSteps*TT::width> packed_thr;
    packed_thr = weight.read();
    // slicer to get 1 PE's worth of thresholds
    auto const pe_slicer = Slice<ap_uint<NumSteps*TT::width>>()(packed_thr);

    TI inElem;
    inElem = in.read();
    auto outElem = TDstI().template operator()<TO>();

    for (unsigned pe = 0; pe < PE; pe++)
    {
#pragma HLS UNROLL
      // slicer to get individual thresholds
      auto const thr_slicer = Slice<TT>()(pe_slicer(pe, 0));
      for (unsigned nt = 0; nt < NumSteps; nt++)
      {
#pragma HLS UNROLL
        internal_thr.m_thresholds[pe][0][nt] = thr_slicer(nt, 0);
      }

      auto const act = TSrcI()(inElem);
      outElem(pe,0,1) = internal_thr.activate(0, pe, act(pe,0));
    }
    out.write(outElem);
  }
}