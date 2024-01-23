#define TOTAL_B 16

void Update_layer( int cont_input,  ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> hc_rst[256]){

	for(int i = 0; i < 256; i++){
		hc_rst[i] = cont_input* hc_rst[i];
	}
}