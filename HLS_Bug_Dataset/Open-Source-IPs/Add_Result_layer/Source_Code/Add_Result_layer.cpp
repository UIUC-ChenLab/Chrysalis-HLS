#define TOTAL_B 16

void Add_Result_layer(ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> gate_input_t[1024], ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> xcstatic_rst[1024], ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> Wxc_tm1[1024], ap_fixed <TOTAL_B,5,AP_TRN_ZERO,AP_SAT> Whc_tm1[1024],ap_int<512> *  data )
{


  //float bias_buf[4000];
	for(int i = 0; i < 1024; i+=32){
		for(int j=0;j<32;j++)
		{
			ap_fixed<12,1,AP_TRN_ZERO,AP_SAT> temp;
			temp.range(11,0)=(*data).range(j*12+11,j*12);
			gate_input_t[i+j] = xcstatic_rst[i+j]+Wxc_tm1[i+j]+Whc_tm1[i+j]+temp;
		}
		data++;
	}
  ///////////////////////
  /*FILE *buf_file;
    buf_file = fopen("tx_xc_b.txt","w+");
    for (int iii=0; iii<4000; iii++){
      fprintf(buf_file, "%f\n", (float)bias_buf[iii]);
    }
  fclose(buf_file);*/
  /////////////////////////
}