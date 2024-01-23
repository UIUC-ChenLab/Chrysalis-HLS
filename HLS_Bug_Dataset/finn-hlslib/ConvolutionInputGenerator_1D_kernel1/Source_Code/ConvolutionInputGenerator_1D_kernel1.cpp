void ConvolutionInputGenerator_1D_kernel1(
		hls::stream<ap_uint<SIMD*Input_precision> > & in,
		hls::stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps) {
static_assert(IFMChannels % SIMD == 0, "");
constexpr unsigned COUNTER_WIDTH = clog2(Stride-1) + 1;
constexpr unsigned COUNTER_RESET = Stride - 2;
	for (unsigned int im=0; im<numReps; im++) {
		ap_int<COUNTER_WIDTH> counter_x = -1;
		for (unsigned int x = 0; x < IFMDim; x++) {
			const bool keep_x = counter_x < 0;
			counter_x = keep_x ? ap_int<COUNTER_WIDTH>(COUNTER_RESET) : ap_int<COUNTER_WIDTH>(counter_x - 1);
			for (unsigned int count_simd = 0; count_simd < IFMChannels/SIMD; count_simd++) {
#pragma HLS pipeline style=flp II=1
				ap_uint<SIMD*Input_precision> inElem = in.read();
				if (keep_x) {
					out.write(inElem);
				}
			}
		}
	}
}