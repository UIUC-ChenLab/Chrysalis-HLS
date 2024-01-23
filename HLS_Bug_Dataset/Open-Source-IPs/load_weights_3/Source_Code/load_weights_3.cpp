void load_weights_3(ap_fixed<12,1,AP_TRN_ZERO,AP_SAT>  weight_buf[2][24][16], //solution2: weight_buf[2][12][16]->weight_buf[2][12][16]
	 				hls::stream<int512> &stream512_in)
{
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=3
ap_int<512> stream_temp;
	 for(int coo=0;coo<24;coo++){
		 stream_temp=stream512_in.read();
		for(int ct=0;ct<2;ct++){
#pragma HLS unroll
			for(int cii=0;cii<16;cii+=2){
#pragma HLS unroll
				weight_buf[ct][coo][cii].range(11,0)=stream_temp.range((cii+ct*16)*12+11,(cii+ct*16)*12);
				weight_buf[ct][coo][cii+1].range(11,0)=stream_temp.range((cii+(ct*16)+1)*12+11,(cii+(ct*16)+1)*12);

			}
		}
	}
}