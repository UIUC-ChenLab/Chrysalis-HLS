void Thresholding_Batch(hls::stream<TI> &in,
                        hls::stream<TO> &out,
                        TA const &activation,
                        int const reps)
{

  // how many different rows each neuron will compute
  // alternatively: number of vertical matrix chunks
  constexpr unsigned  NF = NumChannels / PE;

  // everything merged into a common iteration space (one "big" loop instead
  // of smaller nested loops) to get the pipelinening the way we want
  unsigned nf = 0;
  for (unsigned i = 0; i < reps * ImgDim * NF; i++) {
#pragma HLS pipeline style=flp II=1

    TI const  inElem = in.read();
    auto outElem = TDstI().template operator()<TO>();
    for (unsigned pe = 0; pe < PE; pe++)
    {
#pragma HLS UNROLL
      auto const act = TSrcI()(inElem);
      outElem(pe,0,1) = activation.activate(nf, pe, act(pe,0));
    }
    out.write(outElem);
    if (++nf == NF)
    {
      nf = 0;
    }
  }
}