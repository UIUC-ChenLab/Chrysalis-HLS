void updateParameter(
    FeatureType param[NUM_FEATURES],
    FeatureType grad[NUM_FEATURES],
    FeatureType scale)
{
  #pragma HLS INLINE
  const int unroll_factor = PAR_FACTOR;
  #pragma HLS array_partition variable=grad cyclic factor=unroll_factor
  #pragma HLS array_partition variable=param cyclic factor=unroll_factor

  UPDATE: for (int i = 0; i < NUM_FEATURES / PAR_FACTOR; i++)
  {
    #pragma HLS PIPELINE II=1
    UPDATE_INNER: for (int j = 0; j < PAR_FACTOR; j++)
      param[i*PAR_FACTOR+j] += scale * grad[i*PAR_FACTOR+j];
  }
}