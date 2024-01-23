void read_data(VectorDataType  data[NUM_FEATURES / D_VECTOR_SIZE], 
               DataType        training_instance[NUM_FEATURES])
{
  #pragma HLS INLINE

  READ_TRAINING_DATA: for (int i = 0; i < NUM_FEATURES / D_VECTOR_SIZE; i ++ )
  {
    #pragma HLS pipeline II=1
    VectorFeatureType tmp_data = data[i];
    READ_TRAINING_DATA_INNER: for (int j = 0; j < D_VECTOR_SIZE; j ++ )
      training_instance[i * D_VECTOR_SIZE + j].range(DTYPE_TWIDTH-1, 0) = tmp_data.range((j+1)*DTYPE_TWIDTH-1, j*DTYPE_TWIDTH);
  }
}