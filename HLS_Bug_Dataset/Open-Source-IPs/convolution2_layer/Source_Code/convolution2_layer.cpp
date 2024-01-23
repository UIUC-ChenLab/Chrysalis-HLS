#define TOTAL_B 16

void convolution2_layer(ap_fixed <TOTAL_B,12,AP_TRN_ZERO,AP_SAT>  bottom[2][48][31][31], hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,9,AP_TRN_ZERO,AP_SAT> padded_rst[256][15][15])
{
	ap_fixed <TOTAL_B,9,AP_TRN_ZERO,AP_SAT> top[2][128][27][27];
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[2][16][24];//weight_buf=[2][16][16] -> weight_buf=[2][16][16]
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> bias_buff[2][16];//bias_buff=[2][16] -> bias_buff=[2][16]

#pragma HLS ARRAY_PARTITION variable=top dim=1
#pragma HLS ARRAY_PARTITION variable=top cyclic dim=2 factor=16 //need to be 16
#pragma HLS ARRAY_PARTITION variable=weight_buf  dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf   dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf   dim=3

ap_int<512> stream_temp;
	for(int co=0;co<128;co+=16){ //need to be 16
		stream_temp=stream512_in.read();
		for(int coo=0;coo<16;coo++){
#pragma HLS unroll
			bias_buff[0][coo].range(11,0)=stream_temp.range(coo*24+11,coo*24);
			bias_buff[1][coo].range(11,0)=stream_temp.range(coo*24+23,coo*24+12);
		}


		for(int h=0;h<27;h++){
			for(int w=0;w<27;w++)
			{
#pragma HLS pipeline
				for(int coo=0;coo<16;coo++)//need to be 16
				{
#pragma HLS unroll
					top[0][co+coo][h][w]=bias_buff[0][coo];
					top[1][co+coo][h][w]=bias_buff[1][coo];
				}
			}
		}
	}
	//checked top with bias
/*	FILE *conv2b_file;
	conv2b_file = fopen("conv2_b.txt","w+");
	for(int ct=0;ct<2;ct++){
	for(int i=0;i<128;i++){
		for(int j=0; j<27;j++){
		for(int k=0; k<27;k++){
		  fprintf(conv2b_file, "%f\n", (float)top[ct][i][j][k]);
		}
		}
	}
	}
	fclose(conv2b_file);*/

	for(int h=0;h<15;h++)
		for(int w=0;w<15;w++)
		{
			for(int co=0;co<256;co+=16){//need to be 16
#pragma HLS pipeline
				for(int coo=0;coo<16;coo++)//need to be 16
				{
#pragma HLS unroll
					padded_rst[co+coo][h][w]=0;
				}
			}
		}

	for(int ci=0;ci<48;ci+=24) //solution2: 16->24
		for(int co=0;co<128;co+=16)  //need to be 16
			for(int i=0; i<5; i++){
				for(int j=0; j<5; j++){
#pragma HLS dataflow
					load_weights_2(weight_buf,stream512_in);

					for(int h=0;h<27;h++){
						for(int w=0;w<27;w++){
#pragma HLS pipeline
							for(int coo=0;coo<16;coo++){//need to be 16
#pragma HLS unroll
                                for(int ct=0;ct<2;ct++){

                                	/*for(int cii=0;cii<16;cii++){
#pragma HLS unroll
                                		top[ct][co+coo][h][w]+=weight_buf[ct][coo][cii]*bottom[ct][ci+cii][h+i][w+j];
                                	} //Same as fix-point reference*/
#pragma HLS unroll
                                	top[ct][co+coo][h][w]+=compute_engine_2(
											weight_buf[ct][coo][0] , bottom[ct][ci+0][h+i][w+j],
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
                                            weight_buf[ct][coo][15] , bottom[ct][ci+15][h+i][w+j],
                                            weight_buf[ct][coo][16] , bottom[ct][ci+16][h+i][w+j],
                                            weight_buf[ct][coo][17] , bottom[ct][ci+17][h+i][w+j],
                                            weight_buf[ct][coo][18] , bottom[ct][ci+18][h+i][w+j],
                                            weight_buf[ct][coo][19] , bottom[ct][ci+19][h+i][w+j],
                                            weight_buf[ct][coo][20] , bottom[ct][ci+20][h+i][w+j],
                                            weight_buf[ct][coo][21] , bottom[ct][ci+21][h+i][w+j],
                                            weight_buf[ct][coo][22] , bottom[ct][ci+22][h+i][w+j],
                                            weight_buf[ct][coo][23] , bottom[ct][ci+23][h+i][w+j]);

                               }
							}
						}
					}
				}
            }
	/*FILE *conv2_file;
		conv2_file = fopen("Q8_conv2_top.txt","w+");
		for(int ct=0;ct<2;ct++){
		for(int i=0;i<128;i++){
			for(int j=0; j<27;j++){
			for(int k=0; k<27;k++){
			  fprintf(conv2_file, "%f\n", (float)top[ct][i][j][k]);
			}
			}
		}
		}
	fclose(conv2_file);*/




		for(int ii=0;ii<128;ii+=16)//need to be 16 //coo
		for(int h=0;h<13;h++)
			for(int w=0;w<13;w++){
#pragma HLS pipeline
				for(int i=0;i<16;i++) //need to be 16
				{
#pragma HLS unroll
					padded_rst[ii+i][h+1][w+1]=max_engine_2(top[0][ii+i][h*2][w*2],  top[0][ii+i][h*2][w*2+1],  top[0][ii+i][h*2][w*2+2],
															top[0][ii+i][h*2+1][w*2],top[0][ii+i][h*2+1][w*2+1],top[0][ii+i][h*2+1][w*2+2],
															top[0][ii+i][h*2+2][w*2],top[0][ii+i][h*2+2][w*2+1],top[0][ii+i][h*2+2][w*2+2]);

					padded_rst[128+ii+i][h+1][w+1]=max_engine_2(top[1][ii+i][h*2][w*2],  top[1][ii+i][h*2][w*2+1],  top[1][ii+i][h*2][w*2+2],
															    top[1][ii+i][h*2+1][w*2],top[1][ii+i][h*2+1][w*2+1],top[1][ii+i][h*2+1][w*2+2],
															    top[1][ii+i][h*2+2][w*2],top[1][ii+i][h*2+2][w*2+1],top[1][ii+i][h*2+2][w*2+2]);
				}
			}
	/*	FILE *conv2_file;
			conv2_file = fopen("LRCN_test_conv2_pooling_rst.txt","w+");
			for(int i=0;i<256;i++){
				for(int j=0; j<13;j++){
				for(int k=0; k<13;k++){
				  fprintf(conv2_file, "%f\n", (float)padded_rst[i][j+1][k+1]);
				}
				}
			}
		fclose(conv2_file);*/

 }

// Content of called function
void load_weights_2(ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[2][16][24],
		hls::stream<int512> &stream512_in)
{
ap_int<512> stream_temp;
#pragma HLS ARRAY_PARTITION variable=weight_buf   dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf   dim=2
#pragma HLS ARRAY_PARTITION variable=weight_buf   dim=3
	for(int cii=0;cii<24;cii++){ //solution2: 16->24
		stream_temp=stream512_in.read();
		for(int ct=0;ct<2;ct++){
#pragma HLS unroll
			for(int coo=0;coo<16;coo+=2){ //need to be 16
#pragma HLS unroll
				weight_buf[ct][coo][cii].range(11,0)=stream_temp.range((coo+ct*16)*12+11,(coo+ct*16)*12);
				weight_buf[ct][coo+1][cii].range(11,0)=stream_temp.range((coo+(ct*16)+1)*12+11,(coo+(ct*16)+1)*12);
			}
		}

	}
}