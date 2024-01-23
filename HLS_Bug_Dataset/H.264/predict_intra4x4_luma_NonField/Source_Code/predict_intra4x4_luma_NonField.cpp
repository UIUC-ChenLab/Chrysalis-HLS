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