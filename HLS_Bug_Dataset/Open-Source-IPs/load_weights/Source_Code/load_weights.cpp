
#define DEPTH 16

void load_weights(FIX_WT weight_buf[DEPTH],
				  FIX_WT weights[DEPTH][3][3],
				  int i, int j)
{
#pragma HLS ARRAY_PARTITION variable=weights dim=1 factor=16

	for(int coo = 0; coo < 16; coo++){
#pragma HLS unroll
		weight_buf[coo] = weights[coo][i][j];
	}
}