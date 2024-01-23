#define TOTAL_B 16

void LSTMFullconnection_Layer_loop(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> bottom[256], hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> top[1024])
{


	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight[2][8][8];

#pragma HLS ARRAY_PARTITION variable=weight dim=1
#pragma HLS ARRAY_PARTITION variable=weight dim=2
#pragma HLS ARRAY_PARTITION variable=bottom dim=1 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1 cyclic factor=8
	ap_int<512> stream_temp;

    for(int i=0;i<1024;i+=8)
    {
#pragma HLS pipeline
    	for(int j=0;j<8;j++)
    	{
#pragma HLS unroll factor=8
    			top[i+j]=0;
    	}
    }

                for(int c=0;c<256;c+=8){
                  for(int n=0;n<1024 ;n+=16){
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


}