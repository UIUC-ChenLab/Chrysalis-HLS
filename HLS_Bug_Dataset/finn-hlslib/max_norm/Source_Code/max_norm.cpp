void max_norm(
	hls::stream<ap_uint<WI>> &src,
	hls::stream<ap_uint<WO>> &dst
) {
	static_assert(clog2(1+NORMAX) <= WO, "Specified normalized maximum exceeds output range");
	static ap_uint<WO> const  MAX { NORMAX? NORMAX : -1u };

#pragma HLS dataflow disable_start_propagation
	hls::stream<ap_uint<WI>>  buffer;
#pragma HLS stream variable=buffer depth=FM_SIZE

	// Buffer input and scan it for the maximum
	ap_uint<WI>  max = 1;	// Prevent division by zero
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		auto const  x = src.read();
		max = std::max(max, x);
		buffer.write(x);
	}

	// Replay buffer normalizing all values
	for(unsigned  i = 0; i < FM_SIZE; i++) {
#pragma HLS pipeline II=1 style=flp
		ap_uint<WO+WI>   const  a = MAX * buffer.read();
		ap_uint<WO+WI+1> const  b = (a, ap_uint<1>(0));	// div with one fractional binary digit for rounding
		ap_uint<WO+1>    const  q = b / max;
		dst.write(q(WO, 1) + q[0]);
	}

}