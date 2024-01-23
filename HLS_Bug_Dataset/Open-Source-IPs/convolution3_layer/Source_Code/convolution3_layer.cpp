#define TOTAL_B 16

void convolution3_layer(ap_fixed <TOTAL_B,9,AP_TRN_ZERO,AP_SAT>  bottom[256][15][15], hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,8,AP_TRN_ZERO,AP_SAT> top[2][192][15][15])
{
//#pragma HLS INTERFACE m_axi port=data depth=256
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT>  weight_buf[2][24][16];//solution2: weight_buf[2][12][16]->weight_buf[2][12][16]
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT>  bias_buf[2][16];

#pragma HLS ARRAY_PARTITION variable=weight_buf dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=3
#pragma HLS ARRAY_PARTITION variable=bias_buf dim=1
#pragma HLS ARRAY_PARTITION variable=bias_buf dim=2
	ap_int<512> stream_temp;
//#pragma HLS ARRAY_PARTITION variable=bottom_padded cyclic dim=1 factor=4
    for(int co=0;co<192;co+=16) //need to be 24: 12->12
    {
    	 stream_temp=stream512_in.read();
        for(int coo=0;coo<16;coo++) //need to be 12: 12->12
        {
#pragma HLS unroll
			bias_buf[0][coo].range(11,0)=stream_temp.range(coo*24+11,coo*24); //=bias[0][co+coo];
			bias_buf[1][coo].range(11,0)=stream_temp.range(coo*24+23,coo*24+12); //=bias[1][co+coo];
        }


		for(int h=0;h<13;h++)
			for(int w=0;w<13;w++)
            {
                #pragma HLS pipeline
                for(int coo=0;coo<16;coo++) //need to 12 16: 12->12
                {
#pragma HLS unroll
					top[0][co+coo][1+h][1+w]=bias_buf[0][coo];
					top[1][co+coo][1+h][1+w]=bias_buf[1][coo];
                }
            }
    }

    for(int co=0;co<192;co+=24) //need to be 12: 12->12
    {
		for(int w=0;w<15;w++)
        {
#pragma HLS pipeline
			for(int coo=0;coo<24;coo++) //need to be 12: 12->12
			{
#pragma HLS unroll
				top[0][co+coo][0][w]=0;
				top[0][co+coo][14][w]=0;
				top[1][co+coo][0][w]=0;
				top[1][co+coo][14][w]=0;
			}
		}
    }

    for(int co=0;co<192;co+=24) //need to be 12: 12->12
    {
		for(int h=1;h<14;h++)
		{
#pragma HLS pipeline
			for(int coo=0;coo<24;coo++) //need to be 12: 12->12
			{
#pragma HLS unroll
				top[0][co+coo][h][0]=0;
				top[0][co+coo][h][14]=0;
				top[1][co+coo][h][0]=0;
				top[1][co+coo][h][14]=0;
			}
		}
    }

    for (int ci = 0; ci < 256; ci+=16){ //solution2:16->16
    	for(int i=0; i<3; i++){
    		for(int j=0; j<3; j++){
    			for(int co=0;co<192;co+=24){  //need to be 192/12
#pragma HLS dataflow
                    load_weights_3(weight_buf, stream512_in);//need to be 16: 12->12

                    for(int h=0;h<13;h++){
                        for(int w=0;w<13;w++){
#pragma HLS pipeline
                            for (int coo = 0; coo < 24; coo++){ //need to be 16: 12->12
#pragma HLS unroll
                                for(int ct=0;ct<2;ct++){
                                	/*for(int cii=0;cii<16;cii++){
								#pragma HLS unroll
										top[ct][co+coo][1+h][1+w]+=weight_buf[ct][coo][cii]*bottom[ci+cii][h+i][w+j];
										}*/

#pragma HLS unroll
									top[ct][co+coo][1+h][1+w]+=compute_engine_3(
											weight_buf[ct][coo][0] , bottom[ci+0][h+i][w+j],
											weight_buf[ct][coo][1] , bottom[ci+1][h+i][w+j],
											weight_buf[ct][coo][2] , bottom[ci+2][h+i][w+j],
											weight_buf[ct][coo][3] , bottom[ci+3][h+i][w+j],
											weight_buf[ct][coo][4] , bottom[ci+4][h+i][w+j],
											weight_buf[ct][coo][5] , bottom[ci+5][h+i][w+j],
											weight_buf[ct][coo][6] , bottom[ci+6][h+i][w+j],
											weight_buf[ct][coo][7] , bottom[ci+7][h+i][w+j],
											weight_buf[ct][coo][8] , bottom[ci+8][h+i][w+j],
											weight_buf[ct][coo][9] , bottom[ci+9][h+i][w+j],
											weight_buf[ct][coo][10] , bottom[ci+10][h+i][w+j],
											weight_buf[ct][coo][11] , bottom[ci+11][h+i][w+j],
											weight_buf[ct][coo][12] , bottom[ci+12][h+i][w+j],
											weight_buf[ct][coo][13] , bottom[ci+13][h+i][w+j],
											weight_buf[ct][coo][14] , bottom[ci+14][h+i][w+j],
											weight_buf[ct][coo][15] , bottom[ci+15][h+i][w+j]);

                                }
                            }
                        }
                    }
                }
            }
        }
    }
  /*  FILE *conv3_file;
    	conv3_file = fopen("Q8_conv3_top.txt","w+");
    	for(int ct=0;ct<2;ct++){
    	for(int i=0; i<192;i++)
    	{
    		for(int j=0; j<13;j++)
    		{
    		for(int k=0; k<13;k++)
    		{
    		  fprintf(conv3_file, "%f\n", (float)top[ct][i][1+j][1+k]);
    		}
    		}
    	}
    	}
    	fclose(conv3_file);*/

    for(int co=0;co<192;co+=24)//need to be 12: 12->12
    {
		for(int h=0;h<13;h++)
			for(int w=0;w<13;w++)
            {
#pragma HLS pipeline
                for(int coo=0;coo<24;coo++)//need to be 12: 12->12
                {
#pragma HLS unroll
                	if(top[0][co+coo][1+h][1+w]<0)
                		top[0][co+coo][1+h][1+w]=0;
                	if(top[1][co+coo][1+h][1+w]<0)
						top[1][co+coo][1+h][1+w]=0;
                }
            }
    }


}

// Content of called function
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