#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void inter_luma_single
(
 unsigned char Sluma_cur[PicWidthInSamplesL][FrameHeightInSampleL],
 int rMb[4][4],
 int startx,
 int starty,
 unsigned char xfrac,
 unsigned char yfrac,
 unsigned char xoffset,
 unsigned char yoffset,
 unsigned char temp[9][9],
 unsigned char rmbflag)
{
#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2

#pragma HLS ARRAY_PARTITION variable=rMb complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMb complete dim=2

//#pragma HLS PIPELINE
  int sum;
  unsigned char h,b;
  unsigned char G;
  unsigned char bitoff;

  int i,j;
  int x,y;

  for(i=0;i<4;i++)
    #pragma HLS PIPELINE
    for(j=0;j<4;j++)
    {
      if(yfrac>0 && xfrac!=2)
        h=Clip1y((temp[i+2+xoffset][j]-5*temp[i+2+xoffset][j+1]+20*temp[i+2+xoffset][j+2]+20*temp[i+2+xoffset][j+3]-5*temp[i+2+xoffset][j+4]+temp[i+2+xoffset][j+5]+16)>>5);
      else
        h=0;

      if(xfrac>0 && yfrac!=2)
        b=Clip1y((temp[i][j+2+yoffset]-5*temp[i+1][j+2+yoffset]+20*temp[i+2][j+2+yoffset]+20*temp[i+3][j+2+yoffset]-5*temp[i+4][j+2+yoffset]+temp[i+5][j+2+yoffset]+16)>>5);
      else
        b=0;


      if((yfrac>0 && xfrac==2) || (xfrac>0 && yfrac==2))
      {
        sum=0;
        for(x=0;x<6;x++)
        //#pragma HLS UNROLL
          for(y=0;y<6;y++)
          {
            //#pragma HLS UNROLL
            sum=sum+temp[x+i][y+j]*inter_tab[x][y];
          }
        sum=(sum+512)>>10;
      }
      else
        sum=0;

      if( (yfrac==0 && xfrac!=2) || (xfrac==0 && yfrac!=2) ||(xfrac==0 && yfrac==0))
        G=temp[i+2+xfrac/2][j+2+yfrac/2];
      else
        G=0;

      if(xfrac%2==0 && yfrac%2==0 )
        bitoff=0;
      else
        bitoff=1;

      Sluma_cur[startx+i][starty+j]=   Clip1y( rmbflag*rMb[i][j]+((G+h+b+sum+bitoff)>>bitoff) );
    }
}