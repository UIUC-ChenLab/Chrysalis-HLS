void Qdma2Stream_Batch(hls::stream<qdma_axis<DataWidth,0,0,0> > & in, hls::stream<ap_uint<DataWidth> > & out, const unsigned int numReps){
	//TODO: static_assert to ensure DataWidth is power of 2 between 8 and 512
	for (unsigned int image = 0; image < numReps; image++) {
		for (unsigned int word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
			out.write(in.read().get_data());
		}
	}
}