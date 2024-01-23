#define TOTAL_B 16

void convolution1_layer(ap_fixed <TOTAL_B,16,AP_TRN_ZERO,AP_SAT> bottom[3][227][227],
						hls::stream<int512> &stream512_in,
                        ap_fixed <TOTAL_B,12,AP_TRN_ZERO,AP_SAT> padded_rst[2][48][31][31])
{
//#pragma HLS INTERFACE m_axi port=data depth=256
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight_buf[96][3];
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> bias_buf[96];
	ap_fixed <TOTAL_B,12,AP_TRN_ZERO,AP_SAT> top[96][55][55];

#pragma HLS ARRAY_PARTITION variable=bias_buf
#pragma HLS ARRAY_PARTITION variable=top dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=1
#pragma HLS ARRAY_PARTITION variable=weight_buf dim=2
	ap_int<512> stream_temp;

	for(int h=0;h<31;h++){
		for(int w=0;w<31;w++){
#pragma HLS pipeline
			for(int c=0;c<48;c++){
#pragma HLS unroll
				padded_rst[0][c][h][w]=0;
				padded_rst[1][c][h][w]=0;
			}
		}
	}

	//load bias
	for (int i=0;i<3;i++){
		stream_temp=stream512_in.read();
		for(int j=0;j<32;j++){
			bias_buf[i*32+j].range(11,0) = stream_temp.range(j*12+11,j*12);
		}

	}

	for(int h=0;h<55;h++)
		for(int w=0;w<55;w++){
#pragma HLS pipeline
			for(int co=0;co<96;co++){
#pragma HLS unroll
				top[co][h][w]=bias_buf[co];
			}
		}

	for(int i=0; i<11; i++){
		for(int j=0; j<11; j++){
#pragma HLS dataflow
			load_weights(weight_buf,stream512_in);

			//Reading from AXI needs 9 cycles
			for(int h=0;h<55;h++){
				for(int w=0;w<55;w++){
#pragma HLS pipeline
					for(int cii=0;cii<3;cii++){
						for(int coo=0;coo<96;coo+=3){
#pragma HLS unroll
							top[coo+0][h][w] += weight_buf[coo+0][cii]*bottom[cii][h*4+i][w*4+j];
							top[coo+1][h][w] += weight_buf[coo+1][cii]*bottom[cii][h*4+i][w*4+j];
							top[coo+2][h][w] += weight_buf[coo+2][cii]*bottom[cii][h*4+i][w*4+j];
						}
					}
				}
			}
		}
	}

	/*FILE *conv1_file;
	conv1_file = fopen("Q8_conv1_top.txt","w+");
	for(int i=0;i<96;i++){
		for(int j=0; j<55;j++){
		for(int k=0; k<55;k++){
		  fprintf(conv1_file, "%f\n", (float)top[i][j][k]);
		}
		}
	}
	fclose(conv1_file);*/


	for(int ii=0;ii<48;ii+=16)
		for(int h=0;h<27;h++)
			for(int w=0;w<27;w++)
#pragma HLS pipeline
				for(int i=0;i<16;i++){
#pragma HLS unroll
					padded_rst[0][ii+i][h+2][w+2]=max_engine_1(top[ii+i][h*2][w*2],  top[ii+i][h*2][w*2+1],  top[ii+i][h*2][w*2+2],
															   top[ii+i][h*2+1][w*2],top[ii+i][h*2+1][w*2+1],top[ii+i][h*2+1][w*2+2],
															   top[ii+i][h*2+2][w*2],top[ii+i][h*2+2][w*2+1],top[ii+i][h*2+2][w*2+2]);
					padded_rst[1][ii+i][h+2][w+2]=max_engine_1(top[ii+i+48][h*2][w*2],  top[ii+i+48][h*2][w*2+1],  top[ii+i+48][h*2][w*2+2],
															   top[ii+i+48][h*2+1][w*2],top[ii+i+48][h*2+1][w*2+1],top[ii+i+48][h*2+1][w*2+2],
															   top[ii+i+48][h*2+2][w*2],top[ii+i+48][h*2+2][w*2+1],top[ii+i+48][h*2+2][w*2+2]);
				}

}

// Content of called function
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