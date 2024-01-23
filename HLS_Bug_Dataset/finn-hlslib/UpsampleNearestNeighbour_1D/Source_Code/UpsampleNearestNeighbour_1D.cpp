void UpsampleNearestNeighbour_1D(
        hls::stream<ap_uint<NumChannels * In_t::width>> &src,
        hls::stream<ap_uint<NumChannels * In_t::width>> &dst
) {
	static_assert(OFMDim % IFMDim == 0, "OFMDim must be a whole multiple of IFMDim.");
	constexpr unsigned  REPS = OFMDim / IFMDim;

	using  buf_t = ap_uint<NumChannels * In_t::width>;
	buf_t     buf;
	unsigned  rep = 0;
	for(unsigned  i = 0; i < OFMDim; i++) {
#pragma HLS pipeline II=1 style=flp
		if(rep == 0)  buf = src.read();
		dst.write(buf);
		if(++rep == REPS)  rep = 0;
	}
}