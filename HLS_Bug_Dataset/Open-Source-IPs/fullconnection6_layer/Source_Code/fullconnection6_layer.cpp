#define TOTAL_B 16

void fullconnection6_layer(ap_fixed <TOTAL_B,6,AP_TRN_ZERO,AP_SAT> bottom[256][6][6], hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> top[256])
{

//#pragma HLS INTERFACE m_axi port=data depth=256
	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight[2][8][8];

#pragma HLS ARRAY_PARTITION variable=weight dim=1
#pragma HLS ARRAY_PARTITION variable=weight dim=2
#pragma HLS ARRAY_PARTITION variable=bottom dim=1 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1 cyclic factor=8
ap_int<512> stream_temp;
    for(int i=0;i<256;i+=32)
    {
#pragma HLS pipeline
    	stream_temp=stream512_in.read();
    	for(int j=0;j<32;j+=8)
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

                    for(int h=0;h<6 ;h++)
                    for(int w=0;w<6 ;w++){
                      	for(int c=0;c<256 ;c+=16)
                    	for(int n=0;n<256 ;n+=8){ //4096->256
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
                			/*for(int ii=0;ii<8;ii++)
                			{
                				top[n+nn]+=weight[0][nn][ii]*bottom[c+ii][h][w];

                			}*/
                			#pragma HLS unroll
                			top[n+nn]+=compute_engine_6(
                					weight[0][nn][0],    bottom[c+0][h][w],
                					weight[0][nn][1],    bottom[c+1][h][w],
									weight[0][nn][2],    bottom[c+2][h][w],
									weight[0][nn][3],    bottom[c+3][h][w],
									weight[0][nn][4],    bottom[c+4][h][w],
									weight[0][nn][5],    bottom[c+5][h][w],
									weight[0][nn][6],    bottom[c+6][h][w],
									weight[0][nn][7],    bottom[c+7][h][w]);
                		}


                		for(int nn=0;nn<8;nn++)
                		{
                			/*for(int ii=0;ii<8;ii++)
							{
								top[n+nn]+=weight[1][nn][ii]*bottom[c+ii+8][h][w];

							}*/

							#pragma HLS unroll
                			top[n+nn]+=compute_engine_6(
                					weight[1][nn][0],    bottom[c+8][h][w],
                					weight[1][nn][1],    bottom[c+9][h][w],
									weight[1][nn][2],    bottom[c+10][h][w],
									weight[1][nn][3],    bottom[c+11][h][w],
									weight[1][nn][4],    bottom[c+12][h][w],
									weight[1][nn][5],    bottom[c+13][h][w],
									weight[1][nn][6],    bottom[c+14][h][w],
									weight[1][nn][7],    bottom[c+15][h][w]);
                		}
                    }

            	}


	for(int i=0;i<256;i++)
	{
#pragma HLS pipeline
		if(top[i]<0) top[i]=0;
	}

	/*FILE *conv6_file;
	    conv6_file = fopen("fc6_rst.txt","w+");
	    	for(int i=0; i<256;i++)
	    	{
	    		 fprintf(conv6_file, "%f\n", (float)top[i]);
	    	}
	    	fclose(conv6_file);*/

}