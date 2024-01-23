#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void inter_luma_double_bizero_skip
(
 unsigned char Sluma0[PicWidthInSamplesL][FrameHeightInSampleL],
 unsigned char Sluma1[PicWidthInSamplesL][FrameHeightInSampleL],
 unsigned char Sluma_cur[PicWidthInSamplesL][FrameHeightInSampleL],
 int startx,
 int starty,
 int xint0,
 int yint0,
 int xint1,
 int yint1,
 int rMb[4][4],
 unsigned char rmbflag)
{
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=2

  int i,j;
  int x0,y0;
  int x1,y1;
  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
    {
      x0=Clip3(0,PicWidthInSamplesL-1,xint0+i);
      y0=Clip3(0,FrameHeightInSampleL-1,yint0+j);
      x1=Clip3(0,PicWidthInSamplesL-1,xint1+i);
      y1=Clip3(0,FrameHeightInSampleL-1,yint1+j);

      Sluma_cur[startx+i][starty+j]= Clip1y(rmbflag*rMb[i][j]+((Sluma0[x0][y0]+Sluma1[x1][y1]+1)>>1) );
    }
}