void compute(FeatureType theta_local[NUM_FEATURES], 
             LabelType training_label,
             DataType training_instance[NUM_FEATURES])
{
  // array for storing gradient
  FeatureType gradient[NUM_FEATURES];

  // step size
  FeatureType step = STEP_SIZE;

  // do dot product with the parameter vector
  FeatureType dot = dotProduct(theta_local, training_instance);
  // do sigmoid function
  FeatureType prob = Sigmoid(dot);
  // compute gradient
  computeGradient(gradient, training_instance, (prob-training_label));
  // update the param vector
  updateParameter(theta_local, gradient, -step);
}

// Content of called function
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

// Content of called function
FeatureType Sigmoid(FeatureType exponent) 
{
  #pragma HLS INLINE
  if (exponent > 4) 
    return 1.0;
  else if (exponent < -4) 
    return 0.0;
  else 
  {
    LutInFixed inLut = (LutInFixed)exponent;
    return useLUT(inLut);
  } 
}

// Content of called function
FeatureType useLUT(FeatureType in) 
{ 
  #pragma HLS INLINE
  IdxFixed index;
  if (in < 0)
  {
    in = -in;
    index = (IdxFixed)LUT_SIZE - (IdxFixed)(((TmpFixed)in) << (LUTIN_TWIDTH - LUTIN_IWIDTH));  
  } 
  else 
    index = ((TmpFixed)in) << (LUTIN_TWIDTH - LUTIN_IWIDTH);  
  return lut[index];
}

// Content of called function
void computeGradient(
    FeatureType grad[NUM_FEATURES],
    DataType    feature[NUM_FEATURES],
    FeatureType scale)
{
  #pragma HLS INLINE
  const int unroll_factor = PAR_FACTOR;
  #pragma HLS array_partition variable=grad cyclic factor=unroll_factor
  #pragma HLS array_partition variable=feature cyclic factor=unroll_factor
  GRAD: for (int i = 0; i < NUM_FEATURES / PAR_FACTOR; i++)
  {
    #pragma HLS PIPELINE II=1
    GRAD_INNER: for (int j = 0; j < PAR_FACTOR; j++)
      grad[i*PAR_FACTOR+j] = (scale * feature[i*PAR_FACTOR+j]);
  }
}

// Content of called function
FeatureType dotProduct(FeatureType param[NUM_FEATURES],
                       DataType    feature[NUM_FEATURES])
{
  #pragma HLS INLINE
  const int unroll_factor = PAR_FACTOR;
  
  #pragma HLS array_partition variable=param cyclic factor=unroll_factor
  #pragma HLS array_partition variable=param cyclic factor=unroll_factor

  FeatureType result = 0;
  DOT: for (int i = 0; i < NUM_FEATURES / PAR_FACTOR; i++)
  {
    #pragma HLS PIPELINE II=1
    DOT_INNER: for(int j = 0; j < PAR_FACTOR; j++) 
    {
      FeatureType term = param[i*PAR_FACTOR+j] * feature[i*PAR_FACTOR+j];
      result += term;
    }
  }
  return result;
}