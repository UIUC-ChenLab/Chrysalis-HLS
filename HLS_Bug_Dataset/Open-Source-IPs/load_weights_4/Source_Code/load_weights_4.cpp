void load_weights_4(ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[2][16][24],//solution3: weight_buf[2][16][12]->weight_buf[2][16][12]
					hls::stream<int512> &stream512_in)
{
ap_int<512> stream_temp;

#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=3
	for(int cii=0;cii<24;cii++){
		stream_temp=stream512_in.read();
		for(int ct=0;ct<2;ct++){
#pragma HLS unroll
			for(int coo=0;coo<16;coo+=2){//need to be 16: 16->16		
#pragma HLS unroll
				weight_buf[ct][coo][cii].range(11,0)=stream_temp.range((coo+ct*16)*12+11,(coo+ct*16)*12);
				weight_buf[ct][coo+1][cii].range(11,0)=stream_temp.range((coo+(ct*16)+1)*12+11,(coo+(ct*16)+1)*12);
			}
		}

	}
}