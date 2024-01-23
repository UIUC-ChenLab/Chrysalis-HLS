#define TOTAL_B 16

int arg_max(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT>  probs[8801]){
	int rst  = -1;
	ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> max = -31;

	for(int i = 0; i < 8801; i++){
		if(max < probs[i]){
			max  = probs[i];
			rst = i;
		}
	}
	return rst;
}