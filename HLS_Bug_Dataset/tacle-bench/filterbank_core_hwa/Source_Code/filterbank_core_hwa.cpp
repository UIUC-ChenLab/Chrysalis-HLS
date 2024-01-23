void filterbank_core_hwa( vec_type r[ 256 ],
		vec_type y[ 256 ],
		vec_type H[ 8 ][ 32 ],
		vec_type F[ 8 ][ 32 ] )
{

#pragma HLS INTERFACE ap_ctrl_hs port=return

#pragma HLS RESOURCE variable=r core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=r

#pragma HLS RESOURCE variable=y core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=y

#pragma HLS RESOURCE variable=H core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=H

#pragma HLS RESOURCE variable=F core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=F

  int i, j, k;

  for ( i = 0; i < 256; i++ ) {
	#pragma HLS PIPELINE
	  y[ i ] = 0;
  }

  for ( i = 0; i < 8; i++ ) {

	  vec_type Vect_H[256]; /* (output of the H) */

	  vec_type Vect_Dn[32]; /* output of the down sampler; */

	  vec_type Vect_Up[256];
     /* output of the up sampler; */
	  //#pragma HLS RESOURCE variable=Vect_Up core=RAM_2P_BRAM

	  vec_type Vect_F[256]; /* this is the output of the */

    /* convolving H */

	// Compared to original code, we have
	// moved some conditions out of the for loop condition
	// and into and if statement
    for ( j = 0; j < 256; j++ ) {
	#pragma HLS PIPELINE
    	Vect_H[j] = 0;
      for ( k = 0; k < 32; k++ ) {
    	  if( j - k < 0) break;
    	  Vect_H[ j ] += H[ i ][ k ] * r[ j - k ];
      }
    }

    /* Down Sampling */
    for ( j = 0; j < 32; j++ ) {
		#pragma HLS PIPELINE
    	Vect_Dn[ j ] = Vect_H[ j * 8 ];
    }

    /* Up Sampling */
    for ( j = 0; j < 256; j++ ) {
		#pragma HLS PIPELINE
    	Vect_Up[j] = 0;
    }

    for ( j = 0; j < 32; j++ ) {
		#pragma HLS PIPELINE
    	Vect_Up[ j * 8 ] = Vect_Dn[ j ];
    }

    /* convolving F */

    for ( j = 0; j < 256; j++ ) {
	#pragma HLS PIPELINE
    	Vect_F[j] = 0;
      for ( k = 0; k < 32 ; k++ ) {
    	  if( j - k < 0) break;
          Vect_F[ j ] += F[ i ][ k ] * Vect_Up[ j - k ];
      }
    }

    /* adding the results to the y matrix */

    for ( j = 0; j < 256; j++ ) {
		#pragma HLS PIPELINE
    	y[ j ] += Vect_F[ j ];
    }
  }
}