#define FrameHeightInSampleC	(FrameHeightInMbs*MbHeight_C)

#define PicWidthInSamplesC	(PicWidthInMBs*MbWidth_C)

void inter_prediction_chroma_subblock_double(
    int rMbC[4][4],
    int mv0[2][2][2],
    int mv1[2][2][2],
    unsigned char Schroma0[PicWidthInSamplesC][FrameHeightInSampleC],
    unsigned char Schroma1[PicWidthInSamplesC][FrameHeightInSampleC],
    unsigned char Schroma_cur[PicWidthInSamplesC][FrameHeightInSampleC],
    int startblkx,
    int startblky,
    unsigned char flag)
{

#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=2

#pragma HLS ARRAY_PARTITION variable=mv0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mv0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mv0 complete dim=3

#pragma HLS ARRAY_PARTITION variable=mv1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mv1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mv1 complete dim=3
//#pragma HLS PIPELINE
  int i,j;
  int x,y;
  int x0,y0;


  unsigned char temp0[3][3];
#pragma HLS ARRAY_PARTITION variable=temp0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp0 complete dim=2
  unsigned char temp1[3][3];
#pragma HLS ARRAY_PARTITION variable=temp1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=temp1 complete dim=2
  unsigned char xfrac0,yfrac0;
  int xint0,yint0;


  unsigned char xfrac1,yfrac1;
  int xint1,yint1;



  for(i=0;i<2;i++)
    for(j=0;j<2;j++)
    {

      xint0=mv0[i][j][0]>>3;
      yint0=mv0[i][j][1]>>3;
      xfrac0=(mv0[i][j][0]&0x07);
      yfrac0=(mv0[i][j][1]&0x07);



      xint1=mv1[i][j][0]>>3;
      yint1=mv1[i][j][1]>>3;
      xfrac1=(mv1[i][j][0]&0x07);
      yfrac1=(mv1[i][j][1]&0x07);

      for(x=0;x<3;x++)
      #pragma HLS UNROLL
        for(y=0;y<3;y++)
        {
          #pragma HLS UNROLL
          x0=Clip3(0,PicWidthInSamplesC-1,startblkx+x+xint0+i*2);
          y0=Clip3(0,FrameHeightInSampleC-1,startblky+y+yint0+j*2);
          temp0[x][y]=Schroma0[x0][y0];
        }

      for(x=0;x<3;x++)
      #pragma HLS UNROLL
        for(y=0;y<3;y++)
        {
          #pragma HLS UNROLL
          x0=Clip3(0,PicWidthInSamplesC-1,startblkx+x+xint1+i*2);
          y0=Clip3(0,FrameHeightInSampleC-1,startblky+y+yint1+j*2);
          temp1[x][y]=Schroma1[x0][y0];
        }


      for(x=0;x<2;x++)
      #pragma HLS UNROLL
        for(y=0;y<2;y++)
        {
          #pragma HLS UNROLL
          Schroma_cur[startblkx+x+i*2][startblky+y+j*2]=Clip1y(flag*rMbC[x+i*2][y+j*2]+
              ( ( (((8-xfrac0)*(8-yfrac0)*temp0[x][y]+xfrac0*(8-yfrac0)*temp0[x+1][y]+(8-xfrac0)*yfrac0*temp0[x][y+1]+xfrac0*yfrac0*temp0[x+1][y+1]+32)>>6 )
                  +(((8-xfrac1)*(8-yfrac1)*temp1[x][y]+xfrac1*(8-yfrac1)*temp1[x+1][y]+(8-xfrac1)*yfrac1*temp1[x][y+1]+xfrac1*yfrac1*temp1[x+1][y+1]+32)>>6 )
                  +1)>>1) );
        }
    }

}