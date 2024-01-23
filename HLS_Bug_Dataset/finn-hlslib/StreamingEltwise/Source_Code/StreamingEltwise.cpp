void StreamingEltwise(
	hls::stream<TStrmIn0> &in0,
	hls::stream<TStrmIn1> &in1,
	hls::stream<TStrmOut> &out,
	Fxn &&f
) {
	constexpr unsigned  TOTAL_FOLD = (Channels / PE) * N;
	// everything merged into a common iteration space (one big loop instead
	// of smaller nested loops) to get the pipelining the way we want
	for(unsigned  i = 0; i < TOTAL_FOLD; i++) {
#pragma HLS pipeline style=flp II=1
		auto const  in0_slice_channels = SliceIn0()(in0.read(), 0);
		auto const  in1_slice_channels = SliceIn1()(in1.read(), 0);
		auto outElem = SliceOut().template operator()<TStrmOut>();
		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			outElem(pe, 0, 1) = f(in0_slice_channels(pe, 0), in1_slice_channels(pe, 0));
		}
		out.write(outElem);
	}
}