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