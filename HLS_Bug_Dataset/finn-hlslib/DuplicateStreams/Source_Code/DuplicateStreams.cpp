void DuplicateStreams(hls::stream<ap_uint<DataWidth> > & in, hls::stream<ap_uint<DataWidth> > & out1,
		hls::stream<ap_uint<DataWidth> > & out2) {
	
	for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
		ap_uint<DataWidth> e = in.read();
		
		out1.write(e);
		out2.write(e);
	}
}