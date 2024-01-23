#define PREDICT_LENGTH 53053

void Predict_stream_reader(ap_int<512> *data, hls::stream<int512> &stream512_out)
{
#pragma HLS STREAM variable=stream512_out depth=16
	for(int i=0;i<PREDICT_LENGTH;i++)
	{
#pragma HLS pipeline
		stream512_out.write(data[i]);
	}

}