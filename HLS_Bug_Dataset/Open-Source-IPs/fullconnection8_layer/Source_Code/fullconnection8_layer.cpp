#define TOTAL_B 16

void fullconnection8_layer(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> bottom[256], hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> top[1000])
{
//#pragma HLS INTERFACE m_axi port=data depth=256

	ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> weight[2][8][8];
	ap_int<512> stream_temp;
#pragma HLS ARRAY_PARTITION variable=weight dim=1
#pragma HLS ARRAY_PARTITION variable=weight dim=2
#pragma HLS ARRAY_PARTITION variable=bottom dim=1 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1 cyclic factor=8

    for(int i=0;i<1000;i+=40)
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



						for(int c=0;c<256;c+=16)
                    	for(int n=0;n<1000 ;n+=8)
                    		{
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
															top[n+nn]+=weight[0][nn][ii]*bottom[c+ii];

														}*/
                        							#pragma HLS unroll
                        	                			top[n+nn]+=compute_engine_8(
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
                        	                			/*for(int ii=0;ii<8;ii++)
														{
															top[n+nn]+=weight[1][nn][ii]*bottom[c+ii+8];

														}*/
                        							#pragma HLS unroll
                        	                			top[n+nn]+=compute_engine_8(
                        	                					weight[1][nn][0],    bottom[c+8],
                        	                					weight[1][nn][1],    bottom[c+9],
                        										weight[1][nn][2],    bottom[c+10],
                        										weight[1][nn][3],    bottom[c+11],
                        										weight[1][nn][4],    bottom[c+12],
                        										weight[1][nn][5],    bottom[c+13],
                        										weight[1][nn][6],    bottom[c+14],
                        										weight[1][nn][7],    bottom[c+15]);
                        	                		}
                    }

/*FILE *conv8_file;
conv8_file = fopen("fc8_rst.txt","w+");
		for(int i=0; i<1000;i++)
		{
			 fprintf(conv8_file, "%f\n", (float)top[i]);
		}
		fclose(conv8_file);*/

//Relu is not needed
/*	for(int i=0;i<1000;i+=8)
	{
#pragma HLS pipeline
		for(int ii=0;ii<8;ii++)
		{

			#pragma HLS unroll
			if(top[i+ii]<0) top[i+ii]=0;
		}
	}*/
}