#define LSTMFC_LOOP_LENGTH 6144

void LSTMFC_loop_stream_reader(ap_int<512> *data, hls::stream<int512> &stream512_out)
{
#pragma HLS STREAM variable=stream512_out depth=16
	for(int i=0;i<LSTMFC_LOOP_LENGTH;i++)
	{
#pragma HLS pipeline
		stream512_out.write(data[i]);
	}

}