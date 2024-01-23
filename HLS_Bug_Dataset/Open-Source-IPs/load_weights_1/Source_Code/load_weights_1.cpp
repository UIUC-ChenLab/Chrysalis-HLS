void load_weights(ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[96][3], hls::stream<int512> &stream512_in)
{
#pragma HLS pipeline
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=2

	//need 9 cycles
	for (int j=0; j<3; j++){
		for(int i=0;i<96;i+=32){
			ap_int<512> stream_temp=stream512_in.read();
			for(int ii=0;ii<32;ii++){
#pragma HLS unroll
				weight_buf[i+ii][j].range(11,0)=stream_temp.range(ii*12+11,ii*12);
			}
		}
	}
}