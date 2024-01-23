#define FrameHeightInSampleC	(FrameHeightInMbs*MbHeight_C)

#define PicWidthInSamplesC	(PicWidthInMBs*MbWidth_C)

void inter_prediction_chroma_subblock_single(
    int rMbC[4][4],
    int mv[2][2][2],
    unsigned char Schroma[PicWidthInSamplesC][FrameHeightInSampleC],
    unsigned char Schroma_cur[PicWidthInSamplesC][FrameHeightInSampleC],
    int startblkx,
    int startblky,
    unsigned flag)
{
#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=2



#pragma HLS ARRAY_PARTITION variable=mv complete dim=1
#pragma HLS ARRAY_PARTITION variable=mv complete dim=2
#pragma HLS ARRAY_PARTITION variable=mv complete dim=3


//#pragma HLS PIPELINE
  int i,j;
  int x,y;
  int x0,y0;


  unsigned char temp[3][3];
#pragma HLS ARRAY_PARTITION variable=temp complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp complete dim=2

  unsigned char xfrac,yfrac;
  int xint,yint;


  for(i=0;i<2;i++)
    for(j=0;j<2;j++)
    {
      xint=mv[i][j][0]>>3;
      yint=mv[i][j][1]>>3;
      xfrac=(mv[i][j][0]&0x07);
      yfrac=(mv[i][j][1]&0x07);

      for(x=0;x<3;x++)
      #pragma HLS UNROLL
        for(y=0;y<3;y++)
        {
          #pragma HLS UNROLL
          x0=Clip3(0,PicWidthInSamplesC-1,startblkx+x+xint+i*2);
          y0=Clip3(0,FrameHeightInSampleC-1,startblky+y+yint+j*2);
          temp[x][y]=Schroma[x0][y0];
        }

      for(x=0;x<2;x++)
      #pragma HLS UNROLL
        for(y=0;y<2;y++)
        {
          #pragma HLS UNROLL
          Schroma_cur[startblkx+x+i*2][startblky+y+j*2]=Clip1y(flag*rMbC[x+i*2][y+j*2]+(((8-xfrac)*(8-yfrac)*temp[x][y]+xfrac*(8-yfrac)*temp[x+1][y]+(8-xfrac)*yfrac*temp[x][y+1]+xfrac*yfrac*temp[x+1][y+1]+32)>>6 ) );

        }
    }

}