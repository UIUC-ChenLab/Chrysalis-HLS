#define TOTAL_B 16

void LSTM_layer(int cont_input, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> gate_input_t[1024],ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> c_tm1[256],ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> hc_tm1[256]){
//#pragma HLS ARRAY_PARTITION variable=gate_input_t cyclic factor=400 dim=1




	ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> it, ft, ot, gt;
	for(int i = 0; i < 256; i++){
#pragma HLS pipeline
		it = sigmoid_hls(gate_input_t[i]);
		ft = sigmoid_hls(gate_input_t[i+256]);
		ot = sigmoid_hls(gate_input_t[i+512]);
		gt = tanh_hls(gate_input_t[i+768]);
		c_tm1[i] = cont_input * (ft*c_tm1[i]) + it*gt;
		hc_tm1[i]  =ot*tanh_hls(c_tm1[i]);
	}
}