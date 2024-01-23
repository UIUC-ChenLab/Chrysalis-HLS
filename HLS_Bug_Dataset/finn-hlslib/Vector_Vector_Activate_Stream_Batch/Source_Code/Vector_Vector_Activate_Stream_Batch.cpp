void Vector_Vector_Activate_Stream_Batch(
	hls::stream<TI> &in,
	hls::stream<TO> &out,
	hls::stream<ap_uint<PE*SIMD*TW::width>> &weights,
	TA  const &activation,
	int const  reps,
	R const &r
) {
	static_assert(SIMD == 1, "SIMD parallelism not yet supported.");

	// how many different rows each neuron will compute
	// alternatively: number of vertical matrix chunks
	constexpr unsigned  NF = Channels / PE;

	// how many synapse groups each row is split into
	// alternatively: number of horizontal matrix chunks
	// always equal to # kernel pixels since no SIMD
	constexpr unsigned  SF = Kernel_2;
	decltype(activation.init(0,0))  accu[MMV][PE];
#pragma HLS ARRAY_PARTITION variable=accu complete dim=0

	// unpacked and packed buffers for weight stream
	unsigned  nf   = 0;
	unsigned  sf   = 0;
	unsigned  tile = 0; // invariant: tile = nf*SF + sf
	// everything merged into a common iteration space (one "big" loop instead
	// of smaller nested loops) to get the pipelinening the way we want
	constexpr unsigned  TOTAL_FOLD = NF * SF ;//* Channels/SIMD;
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

		// Packed and unpacked weight representations
		ap_uint<PE * SIMD * TW::width> const  W_packed = weights.read();
		Weights_Tile<SIMD, TW, PE>  w;
#pragma HLS ARRAY_PARTITION variable=w.m_weights complete dim=0
		for(unsigned pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			w.m_weights[pe] = W_packed((pe+1)*SIMD*TW::width-1, pe*SIMD*TW::width);
		}

		for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
			auto const  wgt = TWeightI()(w[pe]);
			for(unsigned mmv = 0; mmv < MMV; mmv++) {
				auto const  act = TSrcI()(inElem, mmv);
				accu[mmv][pe] += mul(wgt[0], act(pe,mmv), r);
			}
		}

		// keep track of which folded synapse/neuron we are processing
		++tile;
		if(++sf == SF) {
			// produce output and clear accumulators
			auto  outElem = TDstI().template operator()<TO>();
			for(unsigned  pe = 0; pe < PE; pe++) {
#pragma HLS UNROLL
				for(unsigned mmv = 0; mmv < MMV; mmv++) {
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