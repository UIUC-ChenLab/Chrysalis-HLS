#define TOTAL_B 16

void convolution5_layer(ap_fixed <TOTAL_B,7,AP_TRN_ZERO,AP_SAT> bottom[2][192][15][15],hls::stream<int512> &stream512_in,ap_fixed <TOTAL_B,6,AP_TRN_ZERO,AP_SAT> finalout[256][6][6])
{
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[2][8][16];//solution2: 16->16
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> bias_buf[2][8];
	ap_fixed <TOTAL_B,6,AP_TRN_ZERO,AP_SAT> top[2][128][13][13];
	ap_int<512> stream_temp;
//#pragma HLS INTERFACE m_axi port=data depth=256
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=3
#pragma HLS ARRAY_PARTITION variable=bottom dim=1
#pragma HLS ARRAY_PARTITION variable=bottom dim=2 cyclic factor=16 //solution2: 16->24
#pragma HLS ARRAY_PARTITION variable=top dim=2 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1
#pragma HLS ARRAY_PARTITION variable=bias_buf dim=1
#pragma HLS ARRAY_PARTITION variable=bias_buf dim=2
//#pragma HLS ARRAY_PARTITION variable=finalout dim=1 cyclic factor=8
//#pragma HLS ARRAY_PARTITION variable=bottom_padded cyclic dim=1 factor=4
	for(int co=0;co<128;co+=8)
    {
		stream_temp=stream512_in.read();
        for(int coo=0;coo<8;coo++)
        {
#pragma HLS pipeline
            bias_buf[0][coo].range(11,0)=stream_temp.range(coo*24+11,coo*24);
            bias_buf[1][coo].range(11,0)=stream_temp.range(coo*24+23,coo*24+12);
        }


		for(int h=0;h<13;h++)
			for(int w=0;w<13;w++)
            {
#pragma HLS pipeline
                for(int coo=0;coo<8;coo++)
                {
#pragma HLS unroll
					top[0][co+coo][h][w]=bias_buf[0][coo];
					top[1][co+coo][h][w]=bias_buf[1][coo];
                }
            }
    }
/*	FILE *conv5_file;
	conv5_file = fopen("LRCN_test_conv5_bias2.txt","w+");
	for(int i=0; i<128;i++)
	{
		for(int j=0; j<13;j++)
		{
		for(int k=0; k<13;k++)
		{
		  fprintf(conv5_file, "%f\n", (float)top[1][i][j][k]);
		}
		}
	}
	fclose(conv5_file);*/

    for (int ci = 0; ci < 192; ci+=16){//solution2: 16->16
    	for(int i=0; i<3; i++){
    		for(int j=0; j<3; j++){
    			for(int co=0;co<128;co+=8){
#pragma HLS dataflow
                    load_weights_5(weight_buf,stream512_in);


                    for(int h=0;h<13;h++){
                        for(int w=0;w<13;w++){
#pragma HLS pipeline
                            for (int coo = 0; coo < 8; coo++){ //try actuall factor 16
#pragma HLS unroll
                                for(int ct=0;ct<2;ct++){

                                	/*for(int cii=0;cii<16;cii++){
									#pragma HLS unroll
										top[ct][co+coo][h][w]+=weight_buf[ct][coo][cii]*bottom[ct][ci+cii][h+i][w+j];
									}*/
#pragma HLS unroll
									top[ct][co+coo][h][w]+=compute_engine_5(weight_buf[ct][coo][0] , bottom[ct][ci+0][h+i][w+j],//try actuall factor 16
                                                                    weight_buf[ct][coo][1] , bottom[ct][ci+1][h+i][w+j],
                                                                    weight_buf[ct][coo][2] , bottom[ct][ci+2][h+i][w+j],
                                                                    weight_buf[ct][coo][3] , bottom[ct][ci+3][h+i][w+j],
                                                                    weight_buf[ct][coo][4] , bottom[ct][ci+4][h+i][w+j],
                                                                    weight_buf[ct][coo][5] , bottom[ct][ci+5][h+i][w+j],
                                                                    weight_buf[ct][coo][6] , bottom[ct][ci+6][h+i][w+j],
                                                                    weight_buf[ct][coo][7] , bottom[ct][ci+7][h+i][w+j],
                                                                    weight_buf[ct][coo][8] , bottom[ct][ci+8][h+i][w+j],
                                                                    weight_buf[ct][coo][9] , bottom[ct][ci+9][h+i][w+j],
                                                                    weight_buf[ct][coo][10] , bottom[ct][ci+10][h+i][w+j],
																	weight_buf[ct][coo][11] , bottom[ct][ci+11][h+i][w+j],
																	weight_buf[ct][coo][12] , bottom[ct][ci+12][h+i][w+j],
																	weight_buf[ct][coo][13] , bottom[ct][ci+13][h+i][w+j],
																	weight_buf[ct][coo][14] , bottom[ct][ci+14][h+i][w+j],
																	weight_buf[ct][coo][15] , bottom[ct][ci+15][h+i][w+j]);

                                }
                            }
                        }
                    }
                }
            }
        }
    }

   /* FILE *conv5_file1;
    conv5_file1 = fopen("Q8_conv5_rst.txt","w+");
	for(int ct=0; ct<2;ct++)
	for(int i=0; i<128;i++)
	{
		for(int j=0; j<13;j++)
		{
		for(int k=0; k<13;k++)
		{
		  fprintf(conv5_file1, "%f\n", (float)top[ct][i][j][k]);
		}
		}
	}
	fclose(conv5_file1);*/

	for(int ii=0;ii<128;ii+=8)
	for(int h=0;h<6;h++)
		for(int w=0;w<6;w++){
#pragma HLS pipeline
			for(int i=0;i<8;i++)
			{
#pragma HLS unroll
				finalout[ii+i][h][w]=max_engine_5(	top[0][ii+i][h*2][w*2],top[0][ii+i][h*2][w*2+1],top[0][ii+i][h*2][w*2+2],
													top[0][ii+i][h*2+1][w*2],top[0][ii+i][h*2+1][w*2+1],top[0][ii+i][h*2+1][w*2+2],
													top[0][ii+i][h*2+2][w*2],top[0][ii+i][h*2+2][w*2+1],top[0][ii+i][h*2+2][w*2+2]);

				finalout[128+ii+i][h][w]=max_engine_5(	top[1][ii+i][h*2][w*2],top[1][ii+i][h*2][w*2+1],top[1][ii+i][h*2][w*2+2],
														top[1][ii+i][h*2+1][w*2],top[1][ii+i][h*2+1][w*2+1],top[1][ii+i][h*2+1][w*2+2],
														top[1][ii+i][h*2+2][w*2],top[1][ii+i][h*2+2][w*2+1],top[1][ii+i][h*2+2][w*2+2]);

			}
		}

		/*FILE *conv5_file;
		conv5_file = fopen("LRCN_test_conv5_pooling_rst.txt","w+");
		for(int i=0; i<256;i++)
		{
			for(int j=0; j<6;j++)
			{
			for(int k=0; k<6;k++)
			{
			  fprintf(conv5_file, "%f\n", (float)finalout[i][j][k]);
			}
			}
		}
		fclose(conv5_file);*/

}

// Content of called function
void load_weights_5(ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[2][8][16],//solution2: 16->16
		hls::stream<int512> &stream512_in)
{
ap_int<512> stream_temp;
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=3
	for(int coo=0;coo<8;coo++){ //solution2:
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