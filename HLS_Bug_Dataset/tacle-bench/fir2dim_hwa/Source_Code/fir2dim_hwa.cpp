void fir2dim_hwa(float  fir2dim_input[SIZE], float fir2dim_output[IMAGEDIM * IMAGEDIM]) {

#pragma HLS RESOURCE variable=fir2dim_input core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=fir2dim_input

#pragma HLS RESOURCE variable=fir2dim_output core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=fir2dim_output

#pragma HLS INTERFACE ap_ctrl_hs port=return

  float *parray  = &fir2dim_input[ARRAY_OFFSET], *parray2, *parray3 ;

  float *pcoeff  = &fir2dim_input[COEFF_OFFSET];
  float *poutput = &fir2dim_output[0];

  int k, f, i;

  for ( k = 0 ; k < IMAGEDIM ; k++ ) {
	#pragma HLS PIPELINE

	for ( f = 0 ; f < IMAGEDIM ; f++ ) {

	  pcoeff = &fir2dim_input[COEFF_OFFSET] ;
	  parray = &fir2dim_input[k * ARRAYDIM + f + ARRAY_OFFSET] ;
	  parray2 = parray + ARRAYDIM ;
	  parray3 = parray + ARRAYDIM + ARRAYDIM ;

	  *poutput = 0 ;

	  for ( i = 0 ; i < COEFFICIENTS ; i++ ){
		*poutput += *pcoeff++ * *parray++ ;
	  }

	  for ( i = 0 ; i < COEFFICIENTS ; i++ ){
		*poutput += *pcoeff++ * *parray2++ ;
	  }

	  for ( i = 0 ; i < COEFFICIENTS ; i++ ){
		*poutput += *pcoeff++ * *parray3++ ;
	  }

	  poutput++ ;
	}
  }

}