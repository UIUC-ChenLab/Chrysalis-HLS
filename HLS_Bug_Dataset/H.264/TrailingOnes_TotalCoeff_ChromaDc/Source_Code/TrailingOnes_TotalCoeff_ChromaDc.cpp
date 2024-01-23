void TrailingOnes_TotalCoeff_ChromaDc(NALU_t *nalu, unsigned char *TotalCoeff, unsigned char *TrailingZeros)
{
//#pragma HLS PIPELINE

  int i,j;
  int len,cod;
  int a, b, c;
  a = 0;
  b = 0;
  c = 0;
  // load nalu->bit_offset and nalu-buf first, because
  // showbits() does not update them
  int offset = nalu->bit_offset;
  unsigned int *temp = (unsigned int*)&nalu->buf[offset / 8];
  unsigned int temp0 = bytes_reverse_32(*temp);

  for(i=0;i<4;i++) 
  {
    #pragma HLS PIPELINE
    for (j=0;j<5;j++)
    {
      len=lentabDC[i][j];
      cod=codtabDC[i][j];
      unsigned char test = (showbits(len,temp0,offset)==cod);
      a += j * test;
      b += i * test;
      c += len * test;

    }
  }

  *TrailingZeros=b;
  *TotalCoeff=a;

#if _N_HLS_
  fprintf(trace_bit,"%s %*d\t%d\n","TrailingZeros & TotalCoeff",50-strlen("TrailingZeros & TotalCoeff"),b,a);
#endif // _N_HLS_
  nalu->bit_offset+=c;
}