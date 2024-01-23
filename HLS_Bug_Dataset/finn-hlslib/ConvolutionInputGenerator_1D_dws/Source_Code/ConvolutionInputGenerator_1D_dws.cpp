void ConvolutionInputGenerator_1D_dws(
	hls::stream<ap_uint<SIMD*Input_precision>> &in,
	hls::stream<ap_uint<SIMD*Input_precision>> &out,
	unsigned const  numReps,
	R const &r
) {
	static_assert((IFMChannels % SIMD) == 0, "SIMD parallelism must divide the number of IFM channels");
	static_assert(OFMDim_x == (IFMDim_x-ConvKernelDim_x+1), "Unexpected OFM dimension");

	constexpr unsigned  SIMD_COUNT  = IFMChannels / SIMD;
	constexpr unsigned  BUFFER_SIZE = ConvKernelDim_x * SIMD_COUNT;
	constexpr unsigned  OUTPUT_SIZE = OFMDim_x * ConvKernelDim_x * SIMD_COUNT;
	constexpr unsigned  INPUT_SIZE = IFMDim_x * SIMD_COUNT;
	constexpr unsigned  READ_CYCLES = SIMD_COUNT * (ConvKernelDim_x- 1) - (ConvKernelDim_x - 1);

	ap_uint<SIMD*Input_precision>  buffer[BUFFER_SIZE];
	memory_resource(buffer, r);

	for(unsigned  count_image = 0; count_image < numReps; count_image++) {
		unsigned  ocnt = SIMD_COUNT;
		unsigned  wp = 0;
		unsigned  rp = 0;
		unsigned  inp_count = 0;
		unsigned  wcnt = 0;
		for(unsigned  i = 0; i < 1 + READ_CYCLES + OUTPUT_SIZE; i++) {
#pragma HLS pipeline style=flp II=1
			bool const  re = i > READ_CYCLES;
			bool const  we = (i < BUFFER_SIZE) || (ocnt < SIMD_COUNT);
			if(re) {
				out.write(buffer[rp]);
				if(++wcnt == ConvKernelDim_x){
					rp += SIMD_COUNT + 1;
					wcnt = 0;
				}
				else{
					rp += SIMD_COUNT;
				}
				if(rp >= BUFFER_SIZE) rp -= BUFFER_SIZE;
				if(++ocnt == BUFFER_SIZE)  ocnt = 0;
			}
			if(we) {
				if(++inp_count <= INPUT_SIZE){
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