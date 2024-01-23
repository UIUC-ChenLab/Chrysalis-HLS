void Stream2Qdma_Batch(hls::stream<ap_uint<DataWidth> > & in, hls::stream<qdma_axis<DataWidth,0,0,0> > & out, const unsigned int numReps){
	for (unsigned int image = 0; image < numReps; image++) {
		for (unsigned int word = 0; word < NumTotal; word++) {
#pragma HLS pipeline style=flp II=1
			qdma_axis<DataWidth,0,0,0> temp;
			temp.set_data(in.read());
			temp.set_keep(-1);
			temp.set_last(word == NumTotal-1);
			out.write(temp);
		}
	}
}