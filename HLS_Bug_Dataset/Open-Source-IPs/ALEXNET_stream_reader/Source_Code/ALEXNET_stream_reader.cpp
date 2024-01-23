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