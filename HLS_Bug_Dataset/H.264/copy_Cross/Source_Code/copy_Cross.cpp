#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_Cross(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL], int xint, int yint,int xoffset, int yoffset,unsigned char temp[9][9])
{

#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2
//#pragma HLS PIPELINE
  int i,j;
  int x,y;

  for(i=0;i<9;i++)
  #pragma HLS UNROLL
    for(j=0;j<9;j++)
    {
      #pragma HLS UNROLL
      if( (i>1+xoffset&&i<6+xoffset) || (j>1+yoffset&&j<6+yoffset))
      {
        x=Clip3(0,PicWidthInSamplesL-1,xint-2+i);
        y=Clip3(0,FrameHeightInSampleL-1,yint-2+j);
        temp[i][j]=Sluma[x][y];
      }
    }
}