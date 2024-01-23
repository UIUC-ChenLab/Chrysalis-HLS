void DuplicateStreams_Batch(hls::stream<ap_uint<DataWidth> > & in, hls::stream<ap_uint<DataWidth> > & out1,
		hls::stream<ap_uint<DataWidth> > & out2, const unsigned int numReps) {
	for (unsigned int image = 0; image < numReps; image++) {
		DuplicateStreams<DataWidth, NumTotal>(in, out1, out2);
	}
}

// Content of called function
void DuplicateStreams(hls::stream<ap_uint<DataWidth> > & in, hls::stream<ap_uint<DataWidth> > & out1,
		hls::stream<ap_uint<DataWidth> > & out2) {
	
	for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
		ap_uint<DataWidth> e = in.read();
		
		out1.write(e);
		out2.write(e);
	}
}