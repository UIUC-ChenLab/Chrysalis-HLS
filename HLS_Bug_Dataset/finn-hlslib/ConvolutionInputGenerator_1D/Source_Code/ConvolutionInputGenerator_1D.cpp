void ConvolutionInputGenerator_1D(
	hls::stream<ap_uint<SIMD*Input_precision>> &in,
	hls::stream<ap_uint<SIMD*Input_precision>> &out,
	unsigned const  numReps,
	R const &r
) {
	static_assert((IFMChannels % SIMD) == 0, "SIMD parallelism must divide the number of IFM channels");
	static_assert(OFMDim_x == ((IFMDim_x - ConvKernelDim_x) / Stride_x + 1), "Unexpected OFM dimension");

	constexpr unsigned  SIMD_COUNT  = IFMChannels / SIMD;
	constexpr unsigned  BUFFER_SIZE = (ConvKernelDim_x - 1) * SIMD_COUNT;
	constexpr unsigned  OUTPUT_SIZE = OFMDim_x * ConvKernelDim_x * SIMD_COUNT;
	constexpr unsigned  INPUT_SIZE = IFMDim_x * SIMD_COUNT;
	constexpr unsigned  WINDOW_SIZE = ConvKernelDim_x * SIMD_COUNT;
	constexpr unsigned  OCNT_INITIAL = BUFFER_SIZE + (Stride_x - 1);

	ap_uint<SIMD*Input_precision>  buffer[BUFFER_SIZE];
	memory_resource(buffer, r);

	for(unsigned  count_image = 0; count_image < numReps; count_image++) {
		signed  ocnt = OCNT_INITIAL < WINDOW_SIZE ? OCNT_INITIAL : -1;
		unsigned  wp = 0;
		unsigned  rp = 0;
		unsigned  offset = 0;
		unsigned  inp_count = 0;
		for(unsigned  i = 0; i < 1+OUTPUT_SIZE; i++) {
#pragma HLS pipeline style=flp II=1
			bool const  re = i > 0;
			bool const  we = (i < WINDOW_SIZE) || (ocnt < SIMD_COUNT * Stride_x);
			if(re) {
				out.write(buffer[rp]);
				if(++offset == WINDOW_SIZE){
					offset = 0;
					rp += 1 + SIMD_COUNT * (Stride_x - 1);
					if(rp >= BUFFER_SIZE)  rp -= BUFFER_SIZE;
				}
				else{ // Explicit else-block required to work around bug in RTL simulation
					if(++rp >= BUFFER_SIZE)  rp -= BUFFER_SIZE;
				}
				if(++ocnt == WINDOW_SIZE)  ocnt = 0;
			}
			if(we) {
				if (++inp_count <= INPUT_SIZE){
					buffer[wp] = in.read();
					if(++wp == BUFFER_SIZE)  wp = 0;
				}
			}
		}
	}
}

// Content of called function
void memory_resource(T inputBuf, ap_resource_lutram const&){
#pragma HLS BIND_STORAGE variable=inputBuf type=RAM_S2P impl=LUTRAM
}