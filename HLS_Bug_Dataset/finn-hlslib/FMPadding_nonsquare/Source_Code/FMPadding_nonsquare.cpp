void FMPadding_nonsquare(
	hls::stream<ap_uint<SIMD*In_t::width>> &in,
	hls::stream<ap_uint<SIMD*In_t::width>> &out
){
	static_assert(NumChannels%SIMD == 0, "Channel count must be a SIMD multiple.");
	constexpr unsigned  Folding = NumChannels/SIMD;

	for(unsigned  y = 0; y < OutputDim_y; y++) {
		for(unsigned  x = 0; x < OutputDim_x; x++) {
			for(unsigned  sf = 0; sf < Folding; sf++) {
#pragma HLS pipeline style=flp II=1
				ap_uint<SIMD*In_t::width>  outData = 0;

				// Read & forward real data only for non-padding image kernel
				if(
					/* rows */ (PaddingTop  <= y) && (y < OutputDim_y - PaddingBottom) &&
					/* cols */ (PaddingLeft <= x) && (x < OutputDim_x - PaddingRight)
				) {
					outData = in.read();
				}
				out.write(outData);
			}
		}
	}
}