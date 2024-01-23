#define TOTAL_B 16

void Embed_Layer(int word_input, ap_int<512> *  data, ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> input_sentence[256])
{

	//load required embed_weights
	//8801*1000 array is mapped into AXI addr.
	//1000 16-bit data need 32 rows AXI addr.
	//8801*1000 need 281632 rows

	if (word_input < 0 || word_input >= 8801) { printf("illegal input\n"); return; }
	data = data + word_input*8;
	for(int i=0;i<256;i+=32){
		for (int ii=0;ii<32;ii++){
#pragma HLS pipeline
			ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> temp;
			temp.range(11,0)=(*data).range((ii)*12+11,(ii)*12);
			input_sentence[i+ii]=temp;
		}
		data++;
	}
              //printf("choice word: %d", word_input);
              /*FILE *buf_file;
                  buf_file = fopen("tx_stence_buf.txt","w+");
                  for (int iii=0; iii<1000; iii++){
                    fprintf(buf_file, "%f\n", (float)input_sentence[iii]);
                  }
                fclose(buf_file);*/

              ///////////////
}