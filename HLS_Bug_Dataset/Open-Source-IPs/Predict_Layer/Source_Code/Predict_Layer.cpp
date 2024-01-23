#define TOTAL_B 16

void Predict_Layer(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> bottom[256] , hls::stream<int512> &stream512_in,  ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> top [8801])
{


	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight[2][8][8];
	ap_int<512> stream_temp;
#pragma HLS ARRAY_PARTITION variable=weight dim=1
#pragma HLS ARRAY_PARTITION variable=weight dim=2

#pragma HLS ARRAY_PARTITION variable=bottom dim=1 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1 cyclic factor=8

	for(int i=0;i<8800;i+=40)
	{
#pragma HLS pipeline
		stream_temp=stream512_in.read();
    	for(int j=0;j<40;j+=8)
    	{
#pragma HLS unroll
    		for(int k=0;k<8;k++)
    		{
#pragma HLS unroll factor=8
    			ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> temp;
    			temp.range(11,0)=stream_temp.range((j+k)*12+11,(j+k)*12);
    			top[i+j+k]=temp;
    		}
    	}


    }
	stream_temp=stream512_in.read();
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> temp2;
	temp2.range(11,0)=stream_temp.range(11,0);
	top[8800]=temp2;


  ///////////////////////
  /*FILE *buf_file;
    buf_file = fopen("tx_predic_b.txt","w+");
    for (int iii=0; iii<8801; iii++){
      fprintf(buf_file, "%f\n", (float)top[iii]);
    }
  fclose(buf_file);*/
  /////////////////////////


			for( int n = 0; n < 8800; n+=16)
			{
			for(int c = 0; c < 256; c+=8){
#pragma HLS pipeline
				stream_temp=stream512_in.read();
          		for(int i=0;i<8;i++)
				{

			#pragma HLS unroll
						for(int j=0;j<8;j++)
						{
			#pragma HLS unroll
							weight[0][i][j].range(3,0)=stream_temp.range( (i*8+j)*8+3,(i*8+j)*8);
							weight[1][i][j].range(3,0)=stream_temp.range( (i*8+j)*8+7,(i*8+j)*8+4);
						}

					}
          		stream_temp=stream512_in.read();


					for(int i=0;i<8;i++)
					{

			#pragma HLS unroll
						for(int j=0;j<8;j++)
						{
			#pragma HLS unroll
							weight[0][i][j].range(7,4)=stream_temp.range( (i*8+j)*8+3,(i*8+j)*8);
							weight[1][i][j].range(7,4)=stream_temp.range( (i*8+j)*8+7,(i*8+j)*8+4);
						}

					}
					stream_temp=stream512_in.read();

					for(int i=0;i<8;i++)
					{

			#pragma HLS unroll
						for(int j=0;j<8;j++)
						{
			#pragma HLS unroll
							weight[0][i][j].range(11,8)=stream_temp.range( (i*8+j)*8+3,(i*8+j)*8);
							weight[1][i][j].range(11,8)=stream_temp.range( (i*8+j)*8+7,(i*8+j)*8+4);
						}

					}





				for(int nn=0;nn<8;nn++)
				{
					/*for(int cc=0;cc<8;cc++){
						top[n+nn]+=weight[0][nn][cc]*bottom[c+cc];
					}*/
				#pragma HLS unroll
					top[n+nn]+=compute_engine_LSTM_loop(
							weight[0][nn][0],    bottom[c+0],
							weight[0][nn][1],    bottom[c+1],
							weight[0][nn][2],    bottom[c+2],
							weight[0][nn][3],    bottom[c+3],
							weight[0][nn][4],    bottom[c+4],
							weight[0][nn][5],    bottom[c+5],
							weight[0][nn][6],    bottom[c+6],
							weight[0][nn][7],    bottom[c+7]);
				}


				for(int nn=0;nn<8;nn++)
				{
					/*for(int cc=0;cc<8;cc++){
						top[n+8+nn]+=weight[1][nn][cc]*bottom[c+cc];
					}*/
				#pragma HLS unroll
					top[n+8+nn]+=compute_engine_LSTM_loop(
							weight[1][nn][0],    bottom[c+0],
							weight[1][nn][1],    bottom[c+1],
							weight[1][nn][2],    bottom[c+2],
							weight[1][nn][3],    bottom[c+3],
							weight[1][nn][4],    bottom[c+4],
							weight[1][nn][5],    bottom[c+5],
							weight[1][nn][6],    bottom[c+6],
							weight[1][nn][7],    bottom[c+7]);
				}
			}
			}


		for(int c=0;c<256;c+=8)
			{
			stream_temp=stream512_in.read();
				for(int j=0;j<8;j++)
				{
				#pragma HLS unroll
					weight[0][0][j].range(11,0)=stream_temp.range(j*12+11,j*12);
				}

				/*for(int cc=0;cc<8;cc++){
					top[8800]+=weight[0][0][cc]*bottom[c+cc];
				}*/
				top[8800]+=compute_engine_LSTM( weight[0][0][0],    bottom[c+0],
												weight[0][0][1],    bottom[c+1],
												weight[0][0][2],    bottom[c+2],
												weight[0][0][3],    bottom[c+3],
												weight[0][0][4],    bottom[c+4],
												weight[0][0][5],    bottom[c+5],
												weight[0][0][6],    bottom[c+6],
												weight[0][0][7],    bottom[c+7]);
			}



}