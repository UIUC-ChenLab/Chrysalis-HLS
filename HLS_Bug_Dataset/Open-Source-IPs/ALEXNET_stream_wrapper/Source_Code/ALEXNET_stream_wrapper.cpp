
#define TOTAL_B 16

void ALEXNET_stream_wrapper(ap_int<512> *data,ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> fc8_rst[1000])
{
	#pragma HLS ARRAY_PARTITION variable=fc8_rst dim=1 cyclic factor=8
	#pragma HLS dataflow
		hls::stream<int512> stream512;
	#pragma HLS STREAM variable=stream512 depth=16 dim=1
		ALEXNET_stream_reader(data,stream512);
		ALEXNET_stream_body(stream512,fc8_rst);
		return;
}

// Content of called function
#define WORD_LENGTH 141269

void ALEXNET_stream_reader(ap_int<512> *data, hls::stream<int512> &stream512_out)
{
#pragma HLS STREAM variable=stream512_out depth=16
	for(int i=0;i<WORD_LENGTH;i++)
	{
#pragma HLS pipeline
		stream512_out.write(data[i]);
	}
}

// Content of called function

#define TOTAL_B 16

void ALEXNET_stream_body(hls::stream<int512> &stream512_in, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> fc8_rst[1000])
{
#pragma HLS STREAM variable=stream512_in depth=16
#pragma HLS ARRAY_PARTITION variable=fc8_rst dim=1 cyclic factor=8
	ap_int<512> stream_word;


	ap_fixed <TOTAL_B,16,AP_TRN_ZERO,AP_SAT> image[3][227][227];
	#pragma HLS ARRAY_PARTITION variable=image dim=1

	ap_fixed <TOTAL_B,12,AP_TRN_ZERO,AP_SAT> padded_rst_1[2][48][31][31];
#pragma HLS ARRAY_PARTITION variable=padded_rst_1 dim=2 cyclic factor=24
#pragma HLS ARRAY_PARTITION variable=padded_rst_1 dim=1
	ap_fixed <TOTAL_B,9,AP_TRN_ZERO,AP_SAT> padded_rst_2[256][15][15];
#pragma HLS ARRAY_PARTITION variable=padded_rst_2 dim=1 cyclic factor=16
	ap_fixed <TOTAL_B,8,AP_TRN_ZERO,AP_SAT> padded_rst_3[2][192][15][15];
#pragma HLS ARRAY_PARTITION variable=padded_rst_3 dim=1
#pragma HLS ARRAY_PARTITION variable=padded_rst_3 dim=2 cyclic factor=24
	ap_fixed <TOTAL_B,7,AP_TRN_ZERO,AP_SAT> padded_rst_4[2][192][15][15];
#pragma HLS ARRAY_PARTITION variable=padded_rst_4 dim=1
#pragma HLS ARRAY_PARTITION variable=padded_rst_4 dim=2 cyclic factor=16
	ap_fixed <TOTAL_B,6,AP_TRN_ZERO,AP_SAT> finalout[256][6][6];
#pragma HLS ARRAY_PARTITION variable=finalout dim=1 cyclic factor=8

	ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> fc6_rst[256];
#pragma HLS ARRAY_PARTITION variable=fc6_rst dim=1 cyclic factor=8
	ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> fc7_rst[256];
#pragma HLS ARRAY_PARTITION variable=fc7_rst dim=1 cyclic factor=8

#pragma HLS ARRAY_PARTITION variable=fc8_rst dim=1 cyclic factor=8


	for (int i=0; i<3; i++){
		 for(int j=0;j<227;j++){
			 for(int k=0;k<227;k+=32){ //8 step
				 stream_word=stream512_in.read();
				 for(int kk=0;kk<32;kk++){
					 if (k+kk<227)
						 image[i][j][k+kk].range(15,0)=stream_word.range(kk*16+15,kk*16);

				 }
			 }
		 }
	 }
	convolution1_layer(image,stream512_in,padded_rst_1);
	convolution2_layer(padded_rst_1,stream512_in,padded_rst_2);
	convolution3_layer(padded_rst_2,stream512_in,padded_rst_3);
	convolution4_layer(padded_rst_3,stream512_in,padded_rst_4);
	convolution5_layer(padded_rst_4,stream512_in,finalout);
	fullconnection6_layer(finalout, stream512_in,fc6_rst);
	fullconnection7_layer(fc6_rst, stream512_in, fc7_rst);
	fullconnection8_layer(fc7_rst,	stream512_in, fc8_rst);
}