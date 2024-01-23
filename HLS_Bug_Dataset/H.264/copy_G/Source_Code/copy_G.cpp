#define FrameHeightInMbs 30

#define PicWidthInMBs 40

#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_G(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL], int xint, int yint,  unsigned char temp[9][9])
{

#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2
//#pragma HLS PIPELINE
  int i,j;

  for(i=2;i<6;i++)
  #pragma HLS UNROLL
    for(j=2;j<6;j++)
    {
      #pragma HLS UNROLL
      temp[i][j]=Sluma[xint-2+i][yint-2+j];
    }
}