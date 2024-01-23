void streamOut(FeatureType theta_local[NUM_FEATURES], VectorFeatureType theta[NUM_FEATURES / F_VECTOR_SIZE])
{
  #pragma HLS INLINE
  // stream out the result
  STREAM_OUT: for (int i = 0; i < NUM_FEATURES / F_VECTOR_SIZE; i ++ )
  {
    #pragma HLS pipeline II=1
    VectorFeatureType tmp_theta = 0;
    STREAM_OUT_INNER: for (int j = 0; j < F_VECTOR_SIZE; j ++ )
      tmp_theta.range((j+1)*FTYPE_TWIDTH-1, j*FTYPE_TWIDTH) = theta_local[i * F_VECTOR_SIZE + j].range(FTYPE_TWIDTH-1, 0);
    theta[i] = tmp_theta;
  }

}