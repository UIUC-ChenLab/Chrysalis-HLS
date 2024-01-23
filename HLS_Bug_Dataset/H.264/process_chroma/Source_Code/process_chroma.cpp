
#define FrameHeightInMbs 30


#define PicWidthInMBs 40

#define INTERSKIP   3

#define INTRA16x16  1

#define INTRA4x4    0

#define MAX_REFERENCE_PICTURES 3

void process_chroma
(
 unsigned char CodedPatternChroma,
 unsigned char NzChroma[2][PicWidthInMBs*2][FrameHeightInMbs*2],
 unsigned char Imode[PicWidthInMBs][FrameHeightInMbs],
 int mbaddrx,
 int mbaddry,
 NALU_t* nalu,
 char qPc,
 char qPcm6,
 char temp1c,
 char temp2c,
 char temp3c,
 int coeffDCC_0[4][2],
 int coeffDCC_1[4][2],
 char refidx0[2][2],
 char refidx1[2][2],
 int mvd0[4][4][2],
 int mvd1[4][4][2],
 ImageParameters* img,
 unsigned char tmpImode,
 unsigned char predC_0[4][4][4],
 unsigned char predC_1[4][4][4],
 StorablePicture PIC[MAX_REFERENCE_PICTURES]
 )

{
#pragma HLS ARRAY_PARTITION variable=predC_0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=predC_0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=predC_1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=predC_1 complete dim=3

#pragma HLS ARRAY_PARTITION variable=coeffDCC_0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffDCC_0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=coeffDCC_1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffDCC_1 complete dim=2

#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=3

  int nC;
  int coeffACC_0[2][2][4][4];
  int coeffACC_1[2][2][4][4];
#pragma HLS ARRAY_PARTITION variable=coeffACC_0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=coeffACC_0 complete dim=4
#pragma HLS ARRAY_PARTITION variable=coeffACC_1 complete dim=3
#pragma HLS ARRAY_PARTITION variable=coeffACC_1 complete dim=4

  int i,j;
  int rMbC_0[2][2][4][4];
  int rMbC_1[2][2][4][4];
#pragma HLS ARRAY_PARTITION variable=rMbC_0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=rMbC_0 complete dim=4
#pragma HLS ARRAY_PARTITION variable=rMbC_1 complete dim=3
#pragma HLS ARRAY_PARTITION variable=rMbC_1 complete dim=4

  int mvdC0[2][2][2];
  int mvdC1[2][2][2];
#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=3
  int tmpidx0;
  int tmpidx1;
  int x, y;
  //processing CR component of each sample
  //first channel
  for(y=0;y<2;y++)
  for(x=0;x<2;x++)
  {

    if(CodedPatternChroma&0x2)
    {
      nC=nc_Chroma(Imode,NzChroma[0],mbaddrx*2+x,mbaddry*2+y);
      NzChroma[0][mbaddrx*2+x][mbaddry*2+y]=residual_block_cavlc_16(coeffACC_0[x][y],nalu,1,15,nC);
    }
    else
    {
      NzChroma[0][mbaddrx*2+x][mbaddry*2+y]=0;
      for(i=0;i<4;i++)
        #pragma HLS PIPELINE
        for(j=0;j<4;j++)
        {
          rMbC_0[x][y][i][j]=0;
          coeffACC_0[x][y][i][j]=0;
        }
    }
  }

  //second channel
  for(y=0;y<2;y++)
  for(x=0;x<2;x++)
  {

    if(CodedPatternChroma&0x2)
    {
      nC=nc_Chroma(Imode,NzChroma[1],mbaddrx*2+x,mbaddry*2+y);
      NzChroma[1][mbaddrx*2+x][mbaddry*2+y]=residual_block_cavlc_16(coeffACC_1[x][y],nalu,1,15,nC);
    }
    else
    {
      NzChroma[1][mbaddrx*2+x][mbaddry*2+y]=0;
      for(i=0;i<4;i++)
        #pragma HLS PIPELINE
        for(j=0;j<4;j++)
        {
          rMbC_1[x][y][i][j]=0;
          coeffACC_1[x][y][i][j]=0;
        }
    }
  }


  for(y=0;y<2;y++)
  for(x=0;x<2;x++)
  {

    if(CodedPatternChroma&0x3)
    {
      scale_residual4x4_and_trans_inverse(qPc, qPcm6, temp1c, temp2c, temp3c, coeffACC_0[x][y],rMbC_0[x][y], coeffDCC_0[x][y],1);
      scale_residual4x4_and_trans_inverse(qPc, qPcm6, temp1c, temp2c, temp3c, coeffACC_1[x][y],rMbC_1[x][y], coeffDCC_1[x][y],1);
    }
    if(refidx0[x][y]>=0 && refidx1[x][y]>=0)
    {
      LOOP_COPY: for(i=0;i<2;i++)
        #pragma HLS PIPELINE
        for(j=0;j<2;j++)
        {
          mvdC0[i][j][0]=mvd0[x*2+i][y*2+j][0];
          mvdC1[i][j][0]=mvd1[x*2+i][y*2+j][0];
          mvdC0[i][j][1]=mvd0[x*2+i][y*2+j][1];
          mvdC1[i][j][1]=mvd1[x*2+i][y*2+j][1];
        }
      tmpidx0=img->list0[refidx0[x][y]];
      tmpidx1=img->list1[refidx1[x][y]];
    }
    else if(refidx0[x][y]>=0 && refidx1[x][y]<0)
    {
      LOOP_COPY2: for(i=0;i<2;i++)
        #pragma HLS PIPELINE
        for(j=0;j<2;j++)
        {
          mvdC0[i][j][0]=mvd0[x*2+i][y*2+j][0];
          mvdC0[i][j][1]=mvd0[x*2+i][y*2+j][1];
        }
      tmpidx0=img->list0[refidx0[x][y]];
    }
    else if(refidx0[x][y]<0 && refidx1[x][y]>=0)
    {
      LOOP_COPY3: for(i=0;i<2;i++)
        #pragma HLS PIPELINE
        for(j=0;j<2;j++)
        {
          mvdC0[i][j][0]=mvd1[x*2+i][y*2+j][0];
          mvdC0[i][j][1]=mvd1[x*2+i][y*2+j][1];
        }
      tmpidx0=img->list1[refidx1[x][y]];
    }

    if(tmpImode==INTRA4x4 || tmpImode== INTRA16x16)
    {
      write_Chroma(predC_0[x+y*2],rMbC_0[x][y],PIC[img->mem_idx].SChroma_0,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,tmpImode==INTERSKIP );
      write_Chroma(predC_1[x+y*2],rMbC_1[x][y],PIC[img->mem_idx].SChroma_1,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,tmpImode==INTERSKIP );
    }
    else if(refidx0[x][y]>=0 && refidx1[x][y]>=0)
    {
      inter_prediction_chroma_subblock_double(rMbC_0[x][y],mvdC0,mvdC1, PIC[tmpidx0].SChroma_0,PIC[tmpidx1].SChroma_0,PIC[img->mem_idx].SChroma_0,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,(CodedPatternChroma&0x3) !=0);
      inter_prediction_chroma_subblock_double(rMbC_1[x][y],mvdC0,mvdC1, PIC[tmpidx0].SChroma_1,PIC[tmpidx1].SChroma_1,PIC[img->mem_idx].SChroma_1,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,(CodedPatternChroma&0x3) !=0);
    }
    else
    {
      inter_prediction_chroma_subblock_single(rMbC_0[x][y],mvdC0,PIC[tmpidx0].SChroma_0,PIC[img->mem_idx].SChroma_0,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,(CodedPatternChroma&0x3) !=0);
      inter_prediction_chroma_subblock_single(rMbC_1[x][y],mvdC0,PIC[tmpidx0].SChroma_1,PIC[img->mem_idx].SChroma_1,(mbaddrx*2+x)*4,(mbaddry*2+y)*4,(CodedPatternChroma&0x3) !=0);
    }

  }
}

// Content of called function
#define FrameHeightInSampleC	(FrameHeightInMbs*MbHeight_C)

#define PicWidthInSamplesC	(PicWidthInMBs*MbWidth_C)

void write_Chroma
(
 unsigned char pred[4][4],
 int rMb[4][4],
 unsigned char SChroma[PicWidthInSamplesC][FrameHeightInSampleC],
 int startx,
 int starty,
 unsigned char skip
 )
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
      SChroma[startx+i][starty+j]=Clip1y((skip==0)*rMb[i][j]+pred[i][j]);
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

// Content of called function
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

// Content of called function
int Clip3(int x, int y, int z)
{
  if(z < x)
  {
    return x;
  }
  else if (z > y)
  {
    return y;
  }
  else
  {
    return z;
  }
  return 0;
}

// Content of called function
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