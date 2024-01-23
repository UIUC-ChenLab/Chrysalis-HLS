#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void write_luma
(
 unsigned char pred[4][4],
 int rMb[4][4],
 unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL],
 int startx,
 int starty,
 unsigned char skip)
{
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=2

#pragma HLS ARRAY_PARTITION variable=pred complete dim=1
#pragma HLS ARRAY_PARTITION variable=pred complete dim=2
  int i,j;

  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
    {
      #pragma HLS PIPELINE rewind
      Sluma[startx+i][starty+j]=Clip1y( skip*rMb[i][j]+pred[i][j]);
    }


}

// Content of called function
int Clip1y(int x)
{
  /* bidDepth regarded as 8 */
  if(255< x)
  {
    return 255;
  }
  else if (0 > x)
  {
    return 0;
  }
  else
  {
    return x;
  }
  return 0;
}