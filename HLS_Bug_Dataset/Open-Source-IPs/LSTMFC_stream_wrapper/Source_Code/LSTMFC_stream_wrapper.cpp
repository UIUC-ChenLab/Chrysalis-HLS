
#define TOTAL_B 16

void LSTMFC_stream_wrapper(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> bottom[1000], ap_int<512> *data, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> top[1024])
{
#pragma HLS ARRAY_PARTITION variable=bottom dim=1 cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=top dim=1 cyclic factor=8
#pragma HLS dataflow
	hls::stream<int512> stream512;
	LSTMFC_stream_reader(data, stream512);
	LSTMFullconnection_Layer(bottom, stream512, top);




}

// Content of called function
#define LSTMFC_LENGTH 24000

void LSTMFC_stream_reader(ap_int<512> *data, hls::stream<int512> &stream512_out)
{
#pragma HLS STREAM variable=stream512_out depth=16
	for(int i=0;i<LSTMFC_LENGTH;i++)
	{
#pragma HLS pipeline
		stream512_out.write(data[i]);
	}

}