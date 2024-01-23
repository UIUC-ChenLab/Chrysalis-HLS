void PackMultiChanData(
	hls::stream<ap_uint<NumChannels*DataWidth> > & in,
	hls::stream<MultiChanData<NumChannels, DataWidth> > & out,
	const unsigned int numReps
) {
	for(unsigned int r = 0; r < numReps; r++) {
#pragma HLS pipeline style=flp II=1
		ap_uint<NumChannels*DataWidth> e = in.read();
		MultiChanData<NumChannels, DataWidth> o;
		for(unsigned int v = 0; v < NumChannels; v++) {
#pragma HLS UNROLL
			o.data[v] = e(DataWidth*(v+1)-1, DataWidth*v);
		}
		out.write(o);
	}
}