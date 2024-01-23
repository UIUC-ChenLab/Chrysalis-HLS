int minver_hwa(mat_type a[DIM][DIM])
{
#pragma HLS RESOURCE variable=a core=RAM_1P_BRAM
#pragma HLS INTERFACE bram port=a
#pragma HLS INTERFACE ap_ctrl_hs port=return

  int work[ 500 ], i, j, k, iw;
  int r = 0;
  mat_type w, wmax, pivot, api, w1;
  mat_type minver_det;

  if ( DIM < 2 || DIM > 500 || EPS <= 0.0 )
    return ( 999 );

  w1 = 1.0;
  for ( i = 0; i < DIM; i++ ) {
	#pragma HLS PIPELINE
	    work[ i ] = i;
  }

  // This has unknown bound
  for ( k = 0; k < DIM; k++ ) {

    wmax = 0.0;
    for ( i = k; i < DIM; i++ ) {
	#pragma HLS PIPELINE
      w = minver_fabs( a[ i ][ k ] );
      if ( w > wmax ) {
        wmax = w;
        r = i;
      }
    }

    pivot = a[ r ][ k ];
    api = minver_fabs( pivot );
    if ( api <= EPS ) {
      minver_det = w1;
      return ( 1 );
    }
    w1 *= pivot;
    if ( r != k ) {
      w1 = -w;
      iw = work[ k ];
      work[ k ] = work[ r ];
      work[ r ] = iw;
      for ( j = 0; j < DIM; j++ ) {
	  #pragma HLS PIPELINE
        w = a[ k ][ j ];
        a[ k ][ j ] = a[ r ][ j ];
        a[ r ][ j ] = w;
      }
    }

    for ( i = 0; i < DIM; i++ ) {
	#pragma HLS PIPELINE
        a[ k ][ i ] /= pivot;
    }
    // Can this block be optimised
    for ( i = 0; i < DIM; i++ ) {
	#pragma HLS PIPELINE
      if ( i != k ) {
        w = a[ i ][ k ];
        if ( w != 0.0 ) {
          for ( j = 0; j < DIM; j++ ) {
            if ( j != k ) a[ i ][ j ] -= w * a[ k ][ j ];
          }
          a[ i ][ k ] = -w / pivot;
        }
      }
    }
    a[ k ][ k ] = 1.0 / pivot;

  }

  for ( i = 0; i < DIM; ) {

    while ( 1 ) {
	#pragma HLS PIPELINE

      k = work[ i ];

      if ( k == i ) break; // This test passes with this

      iw = work[ k ];
      work[ k ] = work[ i ];
      work[ i ] = iw;

      for ( j = 0; j < DIM; j++ ) {
        w = a [ k ][ i ];
        a[ k ][ i ] = a[ k ][ k ];
        a[ k ][ k ] = w;
      }
    }

    i++;
  }

  minver_det = w1;
}