
#define FrameHeightInMbs 30


#define PicWidthInMBs 40

#define INTRA16x16  1

#define INTRA4x4    0

#define MAX_REFERENCE_PICTURES 3

void process_luma(
    unsigned char x,
    unsigned char y,
    unsigned char k,
    int mbaddrx,
    int mbaddry,
    unsigned char CodedPatternLuma,
    NALU_t* nalu,
    int tmpImode,
    int coeffDCL,
    StorablePicture PIC[MAX_REFERENCE_PICTURES],
    unsigned char Imode[PicWidthInMBs][FrameHeightInMbs],
    char IntraPredMode[PicWidthInMBs*4][FrameHeightInMbs*4],
    unsigned char NzLuma[PicWidthInMBs*4][FrameHeightInMbs*4],
    ImageParameters* img,
    unsigned char predL[4][4],
    char qPm6,
    char qPy,
    char temp1l,
    char temp2l,
    char temp3l,
    char refidx0,
    char refidx1,
    int mvd0[2],
    int mvd1[2],
    unsigned char intra4x4predmode
    )
{
  int nC;
  int i,j;

  int coeffACL[4][4];
#pragma HLS ARRAY_PARTITION variable=coeffACL complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffACL complete dim=2
  unsigned char predL4x4[4][4];
#pragma HLS ARRAY_PARTITION variable=predL4x4 complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffACL complete dim=2
  int xint0,yint0,xfrac0,yfrac0;
  int xint1,yint1,xfrac1,yfrac1;

  int rMbL[4][4];
  unsigned char avaimode;

  int tmpidx0;
  int tmpidx1;
#pragma HLS ARRAY_PARTITION variable=rMbL complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMbL complete dim=2


#pragma HLS ARRAY_PARTITION variable=predL complete dim=1
#pragma HLS ARRAY_PARTITION variable=predL complete dim=2

  unsigned char inter_temp0[9][9];
  unsigned char inter_temp1[9][9];

#pragma HLS ARRAY_PARTITION variable=inter_temp0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=inter_temp0 complete dim=2

#pragma HLS ARRAY_PARTITION variable=inter_temp1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=inter_temp1 complete dim=2

#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=1

  //processing luma component of each block
  if(CodedPatternLuma& (1<<(k/4)))
  {
    nC=nc_Luma(Imode,NzLuma,mbaddrx*4+x,mbaddry*4+y);
    NzLuma[mbaddrx*4+x][mbaddry*4+y]=residual_block_cavlc_16(coeffACL,nalu,(tmpImode==INTRA16x16),15,nC);
    //quant
  }
  else if(tmpImode==INTRA16x16)
  {
    NzLuma[mbaddrx*4+x][mbaddry*4+y]=0;
    for(i=0;i<4;i++)
      #pragma HLS PIPELINE
      for(j=0;j<4;j++)
      {
        coeffACL[i][j]=0;
      }
  }
  else
  {
    NzLuma[mbaddrx*4+x][mbaddry*4+y]=0;
  }

  if( (CodedPatternLuma& (1<<(k/4))) || tmpImode==INTRA16x16)
    scale_residual4x4_and_trans_inverse(qPy, qPm6, temp1l, temp2l, temp3l, coeffACL, rMbL, coeffDCL,(tmpImode==INTRA16x16)); /* 8.5.12.1 */

  if(tmpImode==INTRA4x4)
  {
    avaimode=( (mbaddrx*4+x)>0)*2+((mbaddry*4+y)>0);
    predict_intra4x4_luma_NonField(predL4x4,PIC[img->mem_idx].Sluma,intra4x4predmode,avaimode,(mbaddrx*4+x)*4,(mbaddry*4+y)*4,k);
    write_luma(predL4x4,rMbL,PIC[img->mem_idx].Sluma,(mbaddrx*4+x)*4,(mbaddry*4+y)*4, (CodedPatternLuma& (1<<(k/4))) || tmpImode==INTRA16x16);
  }
  else if(tmpImode==INTRA16x16)
  {
    write_luma(predL,rMbL,PIC[img->mem_idx].Sluma,(mbaddrx*4+x)*4,(mbaddry*4+y)*4, (CodedPatternLuma& (1<<(k/4))) || tmpImode==INTRA16x16);
  }
  else
  {
    if(refidx0>=0 && refidx1<0)
    {
      xint0=(mbaddrx*4+x)*4+(mvd0[0]>>2);
      yint0=(mbaddry*4+y)*4+(mvd0[1]>>2);
      xfrac0=(mvd0[0]&0x03);
      yfrac0=(mvd0[1]&0x03);
      tmpidx0=img->list0[(refidx0>=0)*refidx0];
    }
    else if(refidx1>=0 && refidx0<0)
    {
      xint0=(mbaddrx*4+x)*4+(mvd1[0]>>2);
      yint0=(mbaddry*4+y)*4+(mvd1[1]>>2);
      xfrac0=(mvd1[0]&0x03);
      yfrac0=(mvd1[1]&0x03);
      tmpidx0=img->list1[(refidx1>=0)*refidx1];

    }
    else if(refidx1>=0 && refidx0>=0)
    {
      xint1=(mbaddrx*4+x)*4+(mvd1[0]>>2);
      yint1=(mbaddry*4+y)*4+(mvd1[1]>>2);
      xfrac1=(mvd1[0]&0x03);
      yfrac1=(mvd1[1]&0x03);
      tmpidx1=img->list1[(refidx1>=0)*refidx1];

      xint0=(mbaddrx*4+x)*4+(mvd0[0]>>2);
      yint0=(mbaddry*4+y)*4+(mvd0[1]>>2);
      xfrac0=(mvd0[0]&0x03);
      yfrac0=(mvd0[1]&0x03);
      tmpidx0=img->list0[(refidx0>=0)*refidx0];
    }
    if(refidx0>=0 && refidx1>=0 && xfrac0==0 && yfrac0==0 && xfrac1==0 && yfrac1==0 )
    {

      inter_luma_double_bizero_skip
        (
         PIC[tmpidx0].Sluma,
         PIC[tmpidx1].Sluma,
         PIC[img->mem_idx].Sluma,
         (mbaddrx*4+x)*4,
         (mbaddry*4+y)*4,
         xint0,
         yint0,
         xint1,
         yint1,
         rMbL,
         (CodedPatternLuma&(1<<(k/4)))!=0
        );
    }
    else if(refidx0>=0 && refidx1>=0)
    {
      copy_comp(PIC[tmpidx0].Sluma, xint0, yint0,xfrac0,yfrac0, inter_temp0);
      copy_comp(PIC[tmpidx1].Sluma, xint1, yint1,xfrac1,yfrac1, inter_temp1);

      inter_luma_double_skip
        (
         PIC[img->mem_idx].Sluma,
         (mbaddrx*4+x)*4,
         (mbaddry*4+y)*4,
         xfrac0,
         yfrac0,
         xfrac1,
         yfrac1,
         (xfrac0==3 && yfrac0!=0),
         (yfrac0==3 && xfrac0!=0),
         (xfrac1==3 && yfrac1!=0),
         (yfrac1==3 && xfrac1!=0),
         inter_temp0,
         inter_temp1,
         rMbL,
         (CodedPatternLuma&(1<<(k/4)))!=0
        );

    }
    else
    {
      copy_comp(PIC[tmpidx0].Sluma, xint0, yint0,xfrac0,yfrac0,inter_temp0);
      inter_luma_single
        (
         PIC[img->mem_idx].Sluma,
         rMbL,
         (mbaddrx*4+x)*4,
         (mbaddry*4+y)*4,
         xfrac0,
         yfrac0,
         (xfrac0==3 && yfrac0!=0),
         (yfrac0==3 && xfrac0!=0),
         inter_temp0,
         (CodedPatternLuma&(1<<(k/4)))!=0
        );
    }
  }
}

// Content of called function
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
#define HOR_UP_PRED           8

#define VERT_LEFT_PRED        7

#define HOR_DOWN_PRED         6

#define VERT_RIGHT_PRED       5

#define DIAG_DOWN_RIGHT_PRED  4

#define DIAG_DOWN_LEFT_PRED   3

#define DC_PRED               2

#define HOR_PRED              1


#define VERT_PRED             0

#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void predict_intra4x4_luma_NonField(
    unsigned char predL[4][4],
    unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL],
    unsigned char predmode,
    unsigned char avaiMode,
    int startx,
    int starty,
    unsigned int blk)
{
//#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=predL complete dim=1
#pragma HLS ARRAY_PARTITION variable=predL complete dim=2


  unsigned char P_X, P_A, P_B,P_C,P_D,P_E,P_F,P_G,P_H,P_I,P_J,P_K,P_L;


  unsigned char s0;
  if (avaiMode& 0x01)
  {
    P_A = Sluma[startx][starty-1];
    P_B = Sluma[startx+1][starty-1];
    P_C = Sluma[startx+2][starty-1];
    P_D = Sluma[startx+3][starty-1];
  }
  else
  {
    P_A = P_B = P_C = P_D = 128;
  }

  int i,j;
  if( blk==3 || blk==11  || blk==13 || blk==7  || blk==15 || !(avaiMode & 0x01) || startx+4 >= PicWidthInSamplesL || starty==0)
  {
    P_E = P_F = P_G = P_H = P_D;
  }
  else
  {
    P_E = Sluma[startx+4][starty-1];
    P_F = Sluma[startx+5][starty-1];
    P_G = Sluma[startx+6][starty-1];
    P_H = Sluma[startx+7][starty-1];
  }

  if (avaiMode& 0x02)
  {
    P_I = Sluma[startx-1][starty];
    P_J = Sluma[startx-1][starty+1];
    P_K = Sluma[startx-1][starty+2];
    P_L = Sluma[startx-1][starty+3];
  }
  else
  {
    P_I = P_J = P_K = P_L = 128;
  }
  if(avaiMode == 0x03)
  {
    P_X=Sluma[startx-1][starty-1];
  }
  else
  {
    P_X=128;
  }

  switch (predmode)
  {
    case DC_PRED:                         /* DC prediction */

      s0 = 0;
      switch (avaiMode)
      {
        case 3:
          {
            // no edge
            s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)>>3;
          }
          break;
        case 2:
          {
            // upper edge
            s0 = (P_I + P_J + P_K + P_L + 2)>>2;
          }
          break;
        case 1:
          {
            // left edge
            s0 = (P_A + P_B + P_C + P_D + 2)>>2;
          }
          break;
        default:
          {
            // top left corner, nothing to predict from
            s0 = 128;
          }
          break;
      }

      for (j=0; j < 4; j++)
      {
        #pragma HLS UNROLL
        for (i=0; i <4; i++)
        {
          #pragma HLS UNROLL
          // store DC prediction
          predL[i][j] = s0;
        }
      }
      break;


    case VERT_PRED:                       /* vertical prediction from block above */


      for(i=0;i<4;i++)
      {
        #pragma HLS UNROLL
        predL[0][i]=P_A;
        predL[1][i]=P_B;
        predL[2][i]=P_C;
        predL[3][i]=P_D;
      }
      break;

    case HOR_PRED:  /* horizontal prediction from left block */

      for(i=0;i<4;i++)
      {
        #pragma HLS UNROLL
        predL[i][0]=P_I;
        predL[i][1]=P_J;
        predL[i][2]=P_K;
        predL[i][3]=P_L;
      }
      break;

    case DIAG_DOWN_RIGHT_PRED:
      // if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
      //  printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][3 ] = (P_L + 2*P_K + P_J + 2) / 4;
      predL[0 ][2 ] =
      predL[1 ][3 ] = (P_K + 2*P_J + P_I + 2) / 4;
      predL[0 ][1 ] =
      predL[1 ][2 ] =
      predL[2 ][3 ] = (P_J + 2*P_I + P_X + 2) / 4;
      predL[0 ][0 ] =
      predL[1 ][1 ] =
      predL[2 ][2 ] =
      predL[3 ][3 ] = (P_I + 2*P_X + P_A + 2) / 4;
      predL[1 ][0 ] =
      predL[2 ][1 ] =
      predL[3 ][2 ] = (P_X + 2*P_A + P_B + 2) / 4;
      predL[2 ][0 ] =
      predL[3 ][1 ] = (P_A + 2*P_B + P_C + 2) / 4;
      predL[3 ][0 ] = (P_B + 2*P_C + P_D + 2) / 4;
      break;

    case DIAG_DOWN_LEFT_PRED:
      //if (!block_available_up)
      // printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][0 ] = (P_A + P_C + 2*(P_B) + 2) / 4;
      predL[1 ][0 ] =
      predL[0 ][1 ] = (P_B + P_D + 2*(P_C) + 2) / 4;
      predL[2 ][0 ] =
      predL[1 ][1 ] =
      predL[0 ][2 ] = (P_C + P_E + 2*(P_D) + 2) / 4;
      predL[3 ][0 ] =
      predL[2 ][1 ] =
      predL[1 ][2 ] =
      predL[0 ][3 ] = (P_D + P_F + 2*(P_E) + 2) / 4;
      predL[3 ][1 ] =
      predL[2 ][2 ] =
      predL[1 ][3 ] = (P_E + P_G + 2*(P_F) + 2) / 4;
      predL[3 ][2 ] =
      predL[2 ][3 ] = (P_F + P_H + 2*(P_G) + 2) / 4;
      predL[3 ][3 ] = (P_G + 3*(P_H) + 2) / 4;
      break;

    case  VERT_RIGHT_PRED:/* diagonal prediction -22.5 deg to horizontal plane */
      // if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
      // printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][0 ] =
      predL[1 ][2 ] = (P_X + P_A + 1) / 2;
      predL[1 ][0 ] =
      predL[2 ][2 ] = (P_A + P_B + 1) / 2;
      predL[2 ][0 ] =
      predL[3 ][2 ] = (P_B + P_C + 1) / 2;
      predL[3 ][0 ] = (P_C + P_D + 1) / 2;
      predL[0 ][1 ] =
      predL[1 ][3 ] = (P_I + 2*P_X + P_A + 2) / 4;
      predL[1 ][1 ] =
      predL[2 ][3 ] = (P_X + 2*P_A + P_B + 2) / 4;
      predL[2 ][1 ] =
      predL[3 ][3 ] = (P_A + 2*P_B + P_C + 2) / 4;
      predL[3 ][1 ] = (P_B + 2*P_C + P_D + 2) / 4;
      predL[0 ][2 ] = (P_X + 2*P_I + P_J + 2) / 4;
      predL[0 ][3 ] = (P_I + 2*P_J + P_K + 2) / 4;
      break;

    case  VERT_LEFT_PRED:/* diagonal prediction -22.5 deg to horizontal plane */
      // if (!block_available_up)
      //  printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][0 ] = (P_A + P_B + 1) / 2;
      predL[1 ][0 ] =
      predL[0 ][2 ] = (P_B + P_C + 1) / 2;
      predL[2 ][0 ] =
      predL[1 ][2 ] = (P_C + P_D + 1) / 2;
      predL[3 ][0 ] =
      predL[2 ][2 ] = (P_D + P_E + 1) / 2;
      predL[3 ][2 ] = (P_E + P_F + 1) / 2;
      predL[0 ][1 ] = (P_A + 2*P_B + P_C + 2) / 4;
      predL[1 ][1 ] =
      predL[0 ][3 ] = (P_B + 2*P_C + P_D + 2) / 4;
      predL[2 ][1 ] =
      predL[1 ][3 ] = (P_C + 2*P_D + P_E + 2) / 4;
      predL[3 ][1 ] =
      predL[2 ][3 ] = (P_D + 2*P_E + P_F + 2) / 4;
      predL[3 ][3 ] = (P_E + 2*P_F + P_G + 2) / 4;
      break;

    case  HOR_UP_PRED:/* diagonal prediction -22.5 deg to horizontal plane */
      //if (!block_available_left)
      // printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][0 ] = (P_I + P_J + 1) / 2;
      predL[1 ][0 ] = (P_I + 2*P_J + P_K + 2) / 4;
      predL[2 ][0 ] =
      predL[0 ][1 ] = (P_J + P_K + 1) / 2;
      predL[3 ][0 ] =
      predL[1 ][1 ] = (P_J + 2*P_K + P_L + 2) / 4;
      predL[2 ][1 ] =
      predL[0 ][2 ] = (P_K + P_L + 1) / 2;
      predL[3 ][1 ] =
      predL[1 ][2 ] = (P_K + 2*P_L + P_L + 2) / 4;
      predL[3 ][2 ] =
      predL[1 ][3 ] =
      predL[0 ][3 ] =
      predL[2 ][2 ] =
      predL[2 ][3 ] =
      predL[3 ][3 ] = P_L;
      break;

    case  HOR_DOWN_PRED:/* diagonal prediction -22.5 deg to horizontal plane */
      // if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
      // printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n",img->current_mb_nr);

      predL[0 ][0 ] =
      predL[2 ][1 ] = (P_X + P_I + 1) / 2;
      predL[1 ][0 ] =
      predL[3 ][1 ] = (P_I + 2*P_X + P_A + 2) / 4;
      predL[2 ][0 ] = (P_X + 2*P_A + P_B + 2) / 4;
      predL[3 ][0 ] = (P_A + 2*P_B + P_C + 2) / 4;
      predL[0 ][1 ] =
      predL[2 ][2 ] = (P_I + P_J + 1) / 2;
      predL[1 ][1 ] =
      predL[3 ][2 ] = (P_X + 2*P_I + P_J + 2) / 4;
      predL[0 ][2 ] =
      predL[2 ][3 ] = (P_J + P_K + 1) / 2;
      predL[1 ][2 ] =
      predL[3 ][3 ] = (P_I + 2*P_J + P_K + 2) / 4;
      predL[0 ][3 ] = (P_K + P_L + 1) / 2;
      predL[1 ][3 ] = (P_J + 2*P_K + P_L + 2) / 4;
      break;

    default:
      printf("Error: illegal intra_4x4 prediction mode: %d\n",predmode);
      break;
  }


}

// Content of called function
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

// Content of called function
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

// Content of called function
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