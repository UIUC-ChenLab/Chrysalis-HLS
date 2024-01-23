void FlattenMultiChanData(
	hls::stream<MultiChanData<NumChannels, DataWidth> > & in,
	hls::stream<ap_uint<NumChannels*DataWidth> > & out,
	const unsigned int numReps
) {
	for(unsigned int r = 0; r < numReps; r++) {
#pragma HLS pipeline style=flp II=1
		MultiChanData<NumChannels, DataWidth> e = in.read();
		ap_uint<NumChannels*DataWidth> o = 0;
		for(unsigned int v = 0; v < NumChannels; v++) {
#pragma HLS UNROLL
			o(DataWidth*(v+1)-1, DataWidth*v) = e.data[v];
		}
		out.write(o);
	}
}