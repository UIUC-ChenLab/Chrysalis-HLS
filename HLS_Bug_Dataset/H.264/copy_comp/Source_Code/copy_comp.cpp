#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_comp(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL],int xint, int yint,unsigned char xfrac, unsigned char yfrac, unsigned char temp[9][9])
{
#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2
  if( xfrac==0)
  {
    copy_V(Sluma,xint,yint,(xfrac==3 && yfrac!=0),temp);
  }
  else if(yfrac==0)
  {
    copy_H(Sluma,xint,yint,(yfrac==3 && xfrac!=0),temp);
  }
  else if(xfrac%2 && yfrac%2)
  {
    copy_Cross(Sluma,xint,yint,(xfrac==3 && yfrac!=0),(yfrac==3 && xfrac!=0),temp);
  }
  else
  {
    copy_j(Sluma,xint,yint,temp);
  }
}

// Content of called function
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

// Content of called function
#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_j(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL], int xint, int yint, unsigned char temp[9][9])
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
      x=Clip3(0,PicWidthInSamplesL-1,xint-2+i);
      y=Clip3(0,FrameHeightInSampleL-1,yint-2+j);
      temp[i][j]=Sluma[x][y];
    }
}

// Content of called function
#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_V(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL],int xint, int yint, int xoffset, unsigned char temp[9][9])
{

#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2
//#pragma HLS PIPELINE
  int i,j;
  int x,y;

  for(i=2;i<6;i++)
  #pragma HLS UNROLL
    for(j=0;j<9;j++)
    {
      #pragma HLS UNROLL
      x=Clip3(0,PicWidthInSamplesL-1,xint-2+i+xoffset);
      y=Clip3(0,FrameHeightInSampleL-1,yint-2+j);
      temp[i+xoffset][j]=Sluma[x][y];
    }
}

// Content of called function
#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void copy_H(unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL],int xint, int yint, int yoffset, unsigned char temp[9][9])
{
#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2
//#pragma HLS PIPELINE
  int i,j;
  int x,y;

  for(i=0;i<9;i++)
  #pragma HLS UNROLL
    for(j=2;j<6;j++)
    {
      #pragma HLS UNROLL
      x=Clip3(0,PicWidthInSamplesL-1,xint-2+i);
      y=Clip3(0,FrameHeightInSampleL-1,yint-2+j+yoffset);
      temp[i][j+yoffset]=Sluma[x][y];
    }
}