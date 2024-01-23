void ConvolutionInputGenerator_2D_kernel1(
		hls::stream<ap_uint<SIMD*Input_precision> > & in,
		hls::stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps) {
static_assert(IFMChannels % SIMD == 0, "");
constexpr unsigned COUNTER_WIDTH = clog2(Stride-1) + 1;
constexpr unsigned COUNTER_RESET = Stride - 2;
constexpr unsigned MULTIPLYING_FACTOR = IFMChannels/SIMD;
	for (unsigned int im=0; im<numReps; im++) {
#pragma HLS performance target_ti=IFMDim*IFMDim*IFMChannels/SIMD
		ap_int<COUNTER_WIDTH> counter_y = -1;
		for (unsigned int y = 0; y < IFMDim; y++) {
			const bool keep_y = counter_y < 0;
			counter_y = keep_y ? ap_int<COUNTER_WIDTH>(COUNTER_RESET) : ap_int<COUNTER_WIDTH>(counter_y - 1);
			ap_int<COUNTER_WIDTH> counter_x = -1;
			for (unsigned int x = 0; x < IFMDim; x++) {
#pragma HLS pipeline style=flp II=IFMChannels/SIMD
				const bool keep_x = counter_x < 0;
				counter_x = keep_x ? ap_int<COUNTER_WIDTH>(COUNTER_RESET) : ap_int<COUNTER_WIDTH>(counter_x - 1);
				for (unsigned int count_simd = 0; count_simd < MULTIPLYING_FACTOR; count_simd++) {
					ap_uint<SIMD*Input_precision> inElem = in.read();
					if (keep_y && keep_x) {
						out.write(inElem);
					}
				}
			}
		}
	}
}