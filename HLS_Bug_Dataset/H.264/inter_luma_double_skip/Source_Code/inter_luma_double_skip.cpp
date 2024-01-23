#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void inter_luma_double_skip
(
 unsigned char Sluma_cur[PicWidthInSamplesL][FrameHeightInSampleL],
 int startx,
 int starty,
 unsigned char xfrac0,
 unsigned char yfrac0,
 unsigned char xfrac1,
 unsigned char yfrac1,
 unsigned char xoffset0,
 unsigned char yoffset0,
 unsigned char xoffset1,
 unsigned char yoffset1,
 unsigned char temp0[9][9],
 unsigned char temp1[9][9],
 int rMb[4][4],
 unsigned char rmbflag
 )
{
#pragma HLS ARRAY_PARTITION variable=temp1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp1 complete dim=2

#pragma HLS ARRAY_PARTITION variable=temp0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp0 complete dim=2

#pragma HLS ARRAY_PARTITION variable=rMb complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=2


//#pragma HLS PIPELINE
  int sum0,sum1;
  unsigned char h0,b0,h1,b1;
  unsigned char G0,G1;
  unsigned char bitoff0,bitoff1;


  int i,j;
  int x,y;


  for(i=0;i<4;i++)
    #pragma HLS PIPELINE
    for(j=0;j<4;j++)
    {
      if(yfrac0>0 && xfrac0!=2)
        h0=Clip1y((temp0[i+2+xoffset0][j]-5*temp0[i+2+xoffset0][j+1]+20*temp0[i+2+xoffset0][j+2]+20*temp0[i+2+xoffset0][j+3]-5*temp0[i+2+xoffset0][j+4]+temp0[i+2+xoffset0][j+5]+16)>>5);
      else
        h0=0;

      if(xfrac0>0 && yfrac0!=2)
        b0=Clip1y((temp0[i][j+2+yoffset0]-5*temp0[i+1][j+2+yoffset0]+20*temp0[i+2][j+2+yoffset0]+20*temp0[i+3][j+2+yoffset0]-5*temp0[i+4][j+2+yoffset0]+temp0[i+5][j+2+yoffset0]+16)>>5);
      else
        b0=0;


      if((yfrac0>0 && xfrac0==2) || (xfrac0>0 && yfrac0==2))
      {
        sum0=0;
        for(x=0;x<6;x++)
        //#pragma HLS UNROLL
          for(y=0;y<6;y++)
          {
            //#pragma HLS UNROLL
            sum0=sum0+temp0[x+i][y+j]*inter_tab[x][y];
          }
        sum0=(sum0+512)>>10;
      }
      else
        sum0=0;

      if( (yfrac0==0 && xfrac0!=2) || (xfrac0==0 && yfrac0!=2) ||(xfrac0==0 && yfrac0==0))
        G0=temp0[i+2+xfrac0/2][j+2+yfrac0/2];
      else
        G0=0;

      if(xfrac0%2==0 && yfrac0%2==0 )
        bitoff0=0;
      else
        bitoff0=1;

      if(yfrac1>0 && xfrac1!=2)
        h1=Clip1y((temp1[i+2+xoffset1][j]-5*temp1[i+2+xoffset1][j+1]+20*temp1[i+2+xoffset1][j+2]+20*temp1[i+2+xoffset1][j+3]-5*temp1[i+2+xoffset1][j+4]+temp1[i+2+xoffset1][j+5]+16)>>5);
      else
        h1=0;

      if(xfrac1>0 && yfrac1!=2)
        b1=Clip1y((temp1[i][j+2+yoffset1]-5*temp1[i+1][j+2+yoffset1]+20*temp1[i+2][j+2+yoffset1]+20*temp1[i+3][j+2+yoffset1]-5*temp1[i+4][j+2+yoffset1]+temp1[i+5][j+2+yoffset1]+16)>>5);
      else
        b1=0;


      if((yfrac1>0 && xfrac1==2) || (xfrac1>0 && yfrac1==2))
      {
        sum1=0;
        for(x=0;x<6;x++)
        //#pragma HLS UNROLL
          for(y=0;y<6;y++)
          {
            //#pragma HLS UNROLL
            sum1=sum1+temp1[x+i][y+j]*inter_tab[x][y];
          }
        sum1=(sum1+512)>>10;
      }
      else
        sum1=0;

      if( (yfrac1==0 && xfrac1!=2) || (xfrac1==0 && yfrac1!=2) ||(xfrac1==0 && yfrac1==0))
        G1=temp1[i+2+xfrac1/2][j+2+yfrac1/2];
      else
        G1=0;

      if(xfrac1%2==0 && yfrac1%2==0 )
        bitoff1=0;
      else
        bitoff1=1;
      Sluma_cur[startx+i][starty+j]=   Clip1y(rmbflag*rMb[i][j]+( ( ((G0+h0+b0+sum0+bitoff0)>>bitoff0)+ ((G1+h1+b1+sum1+bitoff1)>>bitoff1)+1)>>1 )   );
    }
}