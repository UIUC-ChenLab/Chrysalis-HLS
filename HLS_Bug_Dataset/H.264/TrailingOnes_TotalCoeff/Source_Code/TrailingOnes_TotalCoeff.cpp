void TrailingOnes_TotalCoeff(NALU_t *nalu, unsigned char *TotalCoeff, unsigned char *TrailingZeros, unsigned char nC_range)
{
//#pragma HLS PIPELINE

  int i;
  int j;
  int len;
  int cod;
  if(nC_range>3)
  {
    puts("nc_range too large!");
    exit(-1);
  }
  if(nC_range==3)
  {
    cod=u_n(6,nalu);
    *TrailingZeros=cod & 3;
    *TotalCoeff= (cod >>2)+1;

    if(*TrailingZeros>*TotalCoeff)
    {
      *TrailingZeros=0;
      *TotalCoeff=0;
    }
    return;
  }

  int a, b, c;
  a = 0;
  b = 0;
  c = 0;
  // load nalu->bit_offset and nalu-buf first, because
  // showbits() does not update them
  int offset = nalu->bit_offset;
  unsigned int *temp = (unsigned int*)&nalu->buf[offset / 8];
  unsigned int temp0 = bytes_reverse_32(*temp);

  for(j=0;j<4;j++) 
  {
    #pragma HLS PIPELINE
    for(i=0;i<17;i++)
    {
      len=lentab[nC_range][j][i];
      cod=codtab[nC_range][j][i];
      unsigned char test = (showbits(len,temp0,offset)==cod);
      a += j * test;
      b += i * test;
      c += len * test;
    }
  }

  *TrailingZeros=a;
  *TotalCoeff=b;
#if _N_HLS_
  fprintf(trace_bit,"%s %*d\t%d\t%d\n","TrailingZeros & TotalCoeff",50-strlen("TrailingZeros & TotalCoeff"),b,a,nC_range);
#endif // _N_HLS_
  nalu->bit_offset+=c;
}