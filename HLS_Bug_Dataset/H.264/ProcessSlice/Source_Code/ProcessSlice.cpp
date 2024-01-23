
#define FrameHeightInMbs 30


#define PicWidthInMBs 40

#define IPCM16x16   25

#define INTERSKIP   3

#define INTER4x4    2

#define INTRA16x16  1

#define INTRA4x4    0

#define SLICE_TYPE_I 2

#define SLICE_TYPE_B 1

#define MAX_REFERENCE_PICTURES 3

void ProcessSlice
(
 NALU_t* nalu,
 StorablePicture PIC[MAX_REFERENCE_PICTURES],
 StorablePictureInfo  PICINFO[MAX_REFERENCE_PICTURES],
 unsigned char Imode[PicWidthInMBs][FrameHeightInMbs],
 char IntraPredMode[PicWidthInMBs*4][FrameHeightInMbs*4],
 unsigned char NzLuma[PicWidthInMBs*4][FrameHeightInMbs*4],
 unsigned char NzChroma[2][PicWidthInMBs*2][FrameHeightInMbs*2],
 unsigned char constraint_intra_flag,
 slice_header_rbsp_t *SH,
 ImageParameters* img
 )
{
  int qPprev=img->sliceQPY;
  int type=SH->slice_type;

  //Tables
  const int qPCtable[22]={29,30,31,32,32,33,34,34,35,35,36,36,37,37,37,38,38,38,39,39,39,39};
  const int power2[6]={1,2,4,8,16,32};
  const int intratypecutoff[3]={5,23,0};

  //tempvariables
  unsigned char MbType;
  unsigned char MbSkipFlag=0;
  unsigned char PrevSkip=0;
  int MbSkipRun=0;
  unsigned char tmpImode;

  unsigned char nC;

  unsigned char tmpmbtp;
  unsigned char avaimode;
  int tmpidx0=0;
  int tmpidx1=0;

  //tempsyntaxes
  unsigned char coded_block_pattern;
  unsigned char CodedPatternLuma;
  unsigned char CodedPatternChroma;
  unsigned char IntraChromaPredMode;
  unsigned char Intra16x16PredMode;
  int mb_qp_delta;

  //counter
  int i,j,k;
  int x,y;
  int mbaddrx,mbaddry;

  //inter pred info matrix
  char refCOL[2][2];
#pragma HLS ARRAY_PARTITION variable=refCOL complete dim=1
#pragma HLS ARRAY_PARTITION variable=refCOL complete dim=1

  int mvCOL[4][4][2];
#pragma HLS ARRAY_PARTITION variable=mvCOL complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvCOL complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvCOL complete dim=3

  char refidx0[2][2];
  char refidx1[2][2];
#pragma HLS ARRAY_PARTITION variable=refidx0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=refidx0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=refidx1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=refidx1 complete dim=2

  int mvd0[4][4][2];
  int mvd1[4][4][2];
#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvd0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvd1 complete dim=3

  int mvdC0[2][2][2];
  int mvdC1[2][2][2];

#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvdC0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=3
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=mvdC1 complete dim=1

  //intra pred info array
  char intra4x4predmode[16];

  //prediction matrix
  unsigned char predL[16][4][4];
  unsigned char predC_0[4][4][4];
  unsigned char predC_1[4][4][4];
#pragma HLS ARRAY_PARTITION variable=predL complete dim=2
#pragma HLS ARRAY_PARTITION variable=predL complete dim=3

#pragma HLS ARRAY_PARTITION variable=predC_0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=predC_0 complete dim=3
#pragma HLS ARRAY_PARTITION variable=predC_1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=predC_1 complete dim=3

  //quant temp

  char qPm6,qPi,qPy,qPc,qPcm6;
  char temp1l,temp2l,temp3l;
  char temp1c,temp2c,temp3c;
  char scale1,scale2,scale3;
  //residual matrix
  int coeffDCL[4][4];
#pragma HLS ARRAY_PARTITION variable=coeffDCL complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffDCL complete dim=2


  int coeffDCC_0[4][2];
  int coeffDCC_1[4][2];
#pragma HLS ARRAY_PARTITION variable=coeffDCC_0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffDCC_0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=coeffDCC_1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=coeffDCC_1 complete dim=2

  int rMbL[4][4];
  int rMbC[4][4];
#pragma HLS ARRAY_PARTITION variable=rMbL complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMbL complete dim=2

#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=1
#pragma HLS ARRAY_PARTITION variable=rMbC complete dim=2
  //inter mediate residual
  for(mbaddry=0;mbaddry<FrameHeightInMbs;mbaddry++)
    LOOP_MACROBLOCK:for(mbaddrx=0;mbaddrx<PicWidthInMBs;mbaddrx++)
    {
#if _N_HLS_
      fprintf(trace_bit,"\nMbaddr %d\n",mbaddry*PicWidthInMBs+mbaddrx);
      fprintf(prediction_test,"\nMbaddr %d\n",mbaddry*PicWidthInMBs+mbaddrx);
      fprintf(construction_test,"\nMbaddr %d\n",mbaddry*PicWidthInMBs+mbaddrx);
#endif
      if(type!= SLICE_TYPE_I)
      {
        if(MbSkipRun==0 && PrevSkip==0)
        {
          MbSkipRun=u_e(nalu);
#if _N_HLS_
          fprintf(trace_bit,"\n mbskip_run %d\n", MbSkipRun);
#endif
          if(MbSkipRun==0)
          {
            MbSkipFlag=0;
            PrevSkip=0;
          }
          else
          {
            MbSkipFlag=1;
            PrevSkip=1;
            MbSkipRun--;
          }
        }
        else if(MbSkipRun>0)
        {
          MbSkipFlag=1;
          MbSkipRun--;
        }
        else
        {
          MbSkipFlag=0;
          PrevSkip=0;
        }
      }

      // following decide the skip mode
      if(MbSkipFlag==0)
      {
        tmpmbtp=MbType=u_e(nalu);
#if _N_HLS_
        fprintf(trace_bit,"mbtype %d\n",MbType);
#endif // _N_HLS_
        if(MbType>=intratypecutoff[type])
        {
          MbType=MbType-intratypecutoff[type];
          tmpImode=Imode[mbaddrx][mbaddry]=(MbType==0 || MbType==25)?MbType:1;
        }
        else
        {
          MbType=MbType+(type?7:1);
          tmpImode=Imode[mbaddrx][mbaddry]=INTER4x4;
        }
      }
      else
      {
        MbType=(type?6:0);
        tmpImode=Imode[mbaddrx][mbaddry]=INTERSKIP;
        CodedPatternChroma=0;
        CodedPatternLuma=0;
      }

      // following function will process the prediction information
      if(tmpImode==INTER4x4||tmpImode==INTERSKIP)
      {
        // this part will read and calculate the inter prediction information
        //preparing for mvcol
        if(type==SLICE_TYPE_B)
        {
          for(i=0;i<4;i++)
            for(j=0;j<4;j++)
            {

              if( PICINFO[img->list1[0]].refIdxL0[(mbaddrx*4+i)/2][(mbaddry*4+j)/2]>=0)
              {
                refCOL[i/2][j/2]=PICINFO[img->list1[0]].refIdxL0[(mbaddrx*4+i)/2][(mbaddry*4+j)/2];
                mvCOL[i][j][0]=PICINFO[img->list1[0]].mvd_l0[mbaddrx*4+i][mbaddry*4+j][0];
                mvCOL[i][j][1]=PICINFO[img->list1[0]].mvd_l0[mbaddrx*4+i][mbaddry*4+j][1];
              }
              else
              {
                refCOL[i/2][j/2]=PICINFO[img->list1[0]].refIdxL1[(mbaddrx*4+i)/2][(mbaddry*4+j)/2];
                mvCOL[i][j][0]=PICINFO[img->list1[0]].mvd_l1[mbaddrx*4+i][mbaddry*4+j][0];
                mvCOL[i][j][1]=PICINFO[img->list1[0]].mvd_l1[mbaddrx*4+i][mbaddry*4+j][1];
              }
            }

        }
        // Set Intra prediction info
        LOOP_ZERO_INTRAMODED:for(i=0;i<4;i++)
        {
          #pragma HLS UNROLL
          IntraPredMode[mbaddrx*4+3][mbaddry*4+i]=2;
          IntraPredMode[mbaddrx*4+i][mbaddry*4+3]=2;
        }


        processinterMbType(
         tmpmbtp,
         type,
         nalu,
         SH->num_ref_idx_l1_active_minus1,
         SH->num_ref_idx_l0_active_minus1,
         refidx0,
         refidx1,
         mvd0,
         mvd1,
         PICINFO[img->mem_idx].refIdxL0,
         PICINFO[img->mem_idx].refIdxL1,
         PICINFO[img->mem_idx].mvd_l0,
         PICINFO[img->mem_idx].mvd_l1,
         img->list0,
         img->list1,
         mbaddrx+mbaddry*PicWidthInMBs,
         MbSkipFlag,
         refCOL,
         mvCOL,
         MbType);
      }
      else if(tmpImode==INTRA16x16 ||tmpImode==INTRA4x4 )
      {
        // this part will read and calculate the intra prediction information
        if(tmpImode==INTRA4x4)
        {
          IntraInfo(nalu,IntraPredMode,PICINFO[img->mem_idx].refIdxL0,PICINFO[img->mem_idx].refIdxL1,intra4x4predmode, constraint_intra_flag,mbaddrx*4,mbaddry*4);
        }
        else
        {
          for(i=0;i<4;i++)
          {
            #pragma HLS UNROLL
            IntraPredMode[mbaddrx*4+3][mbaddry*4+i]=2;
            IntraPredMode[mbaddrx*4+i][mbaddry*4+3]=2;
          }
        }
        Intra16x16PredMode=(MbType-1)%4;
        IntraChromaPredMode=u_e(nalu);
#if _N_HLS_
        fprintf(trace_bit,"%s %*d\n","intra_chroma_pred_mode",50-strlen("intra_chroma_pred_mode"),IntraChromaPredMode);
#endif // _N_HLS_
      }
      if(tmpImode!=IPCM16x16)
      {
        if(tmpImode!=INTERSKIP)
        {
          if(tmpImode!=INTRA16x16)
          {
            coded_block_pattern=m_e( (tmpImode!=INTRA4x4), nalu);
#if _N_HLS_
            fprintf(trace_bit,"%s %*d\n","coded_block_pattern",50-strlen("coded_block_pattern"),coded_block_pattern);
#endif // _N_HLS_
            CodedPatternLuma=coded_block_pattern%16;
            CodedPatternChroma=coded_block_pattern/16;

          }
          else
          {
            CodedPatternChroma=(MbType-1)/4%3;
            CodedPatternLuma=MbType>12?15:0;
          }

          if(CodedPatternChroma>0 || CodedPatternLuma>0 || tmpImode==INTRA16x16)
          {
            mb_qp_delta=s_e(nalu);
#if _N_HLS_
            fprintf(trace_bit,"%s %*d\n","mb_qp_delta",50-strlen("mb_qp_delta"),mb_qp_delta);
#endif // _N_HLS_
            qPprev+=mb_qp_delta;
            qPy=qPprev;
            qPm6=qPy%6;
            temp1l=qPy/6-4;
            temp2l=4-qPy/6;

            if(temp1l<0)
              temp3l=power2[3-qPy/6];

            scale1=qPy/6-6;
            scale2=6-qPy/6;

            if(scale1<0)
              scale3=power2[5-qPy/6];


            qPi=Clip3(0,51,qPy+img->chroma_offset);

            if(qPi<30)
              qPc=qPi;
            else
              qPc=qPCtable[qPi-30];

            qPcm6=qPc%6;

            temp1c=qPc/6-4;
            temp2c=4-qPc/6;
            temp3c=1<<(3-qPc/6);
          }
        }

        if(tmpImode==INTRA16x16)
        {
          //processing INTRA16x16 DC
          nC=nc_Luma(Imode,NzLuma,mbaddrx*4,mbaddry*4);
          residual_block_cavlc_16(coeffDCL,nalu,0,15,nC);
          //Scaling
          scale_and_inv_trans_Intra16x16DC(qPy,coeffDCL,qPm6,scale1,scale2,scale3);
          predict_intra16x16_luma_NonField(predL,PIC[img->mem_idx].Sluma,Intra16x16PredMode,(mbaddrx>0)*2+(mbaddry>0),mbaddrx*16,mbaddry*16);
        }

        LOOP_LUMA_SUBBLOCK:for(k=0;k<16;k++)
        {
          x=KTOX(k);
          y=KTOY(k);

          //processing luma component of each block
          process_luma(
           x,
           y,
           k,
           mbaddrx,
           mbaddry,
           CodedPatternLuma,
           nalu,
           tmpImode,
           coeffDCL[x][y],
           PIC,
           Imode,
           IntraPredMode,
           NzLuma,
           img,
           predL[k],
           qPm6,
           qPy,
           temp1l,
           temp2l,
           temp3l,
           refidx0[x/2][y/2],
           refidx1[x/2][y/2],
           mvd0[x][y],
           mvd1[x][y],
           intra4x4predmode[k]);
        }
        if(CodedPatternChroma&0x3)
        {
          residual_block_cavlc_4(coeffDCC_0, nalu, 0,3);
          scale_and_inv_trans_chroma2x2(coeffDCC_0,qPc, qPc%6);
          residual_block_cavlc_4(coeffDCC_1, nalu, 0,3);
          scale_and_inv_trans_chroma2x2(coeffDCC_1,qPc, qPc%6);
        }

        if(tmpImode==INTRA16x16 || tmpImode==INTRA4x4)
        {
          prediction_Chroma(predC_0,PIC[img->mem_idx].SChroma_0,(mbaddrx>0)*2+(mbaddry>0),mbaddrx*8,mbaddry*8,IntraChromaPredMode);
          prediction_Chroma(predC_1,PIC[img->mem_idx].SChroma_1,(mbaddrx>0)*2+(mbaddry>0),mbaddrx*8,mbaddry*8,IntraChromaPredMode);
        }
        //processing CR component

        process_chroma(
          CodedPatternChroma,
          NzChroma,
          Imode,
          mbaddrx,
          mbaddry,
          nalu,
          qPc,
          qPcm6,
          temp1c,
          temp2c,
          temp3c,
          coeffDCC_0,
          coeffDCC_1,
          refidx0,
          refidx1,
          mvd0,
          mvd1,
          img,
          tmpImode,
          predC_0,
          predC_1,
          PIC);
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

#define FrameHeightInMbs 30


#define PicWidthInMBs 40

#define MAX_REFERENCE_PICTURES 3

void processinterMbType(
    unsigned char mb_type,
    unsigned char slice_type,
    NALU_t * nalu,
    unsigned char num_ref_idx_active1,
    unsigned char num_ref_idx_active0,
    char refidx0[2][2],
    char refidx1[2][2],
    int mvd0[4][4][2],
    int mvd1[4][4][2],
    char globref0[PicWidthInMBs*2][FrameHeightInMbs*2],
    char globref1[PicWidthInMBs*2][FrameHeightInMbs*2],
    int globmvd0[PicWidthInMBs*4][FrameHeightInMbs*4][2],
    int globmvd1[PicWidthInMBs*4][FrameHeightInMbs*4][2],
    int list0[MAX_REFERENCE_PICTURES],
    int list1[MAX_REFERENCE_PICTURES],
    int Mbaddress,
    int skipflag,
    char refcol[2][2],
    int mvcol[4][4][2],
    unsigned char MbType
)
{
  unsigned char substepx[2][2];
  unsigned char substepy[2][2];
  unsigned char stepx;
  unsigned char stepy;
  unsigned char listuse[2][2];
  unsigned char preddir[2][2];
  unsigned char subtype;
  unsigned char dflag;
  int i,j;

  int x,y;
  int subx,suby;
  char refidx;
  int mvdx;
  int mvdy;
  int tpmmv[2];
  int tpmmv1[2];
  int foundmv0[2];
  int foundmv1[2];
  char tmpref01[2];

  int startblkx= (Mbaddress%PicWidthInMBs)*4;
  int startblky= (Mbaddress/PicWidthInMBs)*4;



  const unsigned char val_stepX[30]=  {4,4,4,2,2,2, // this part is for P slice
    2,2,4,4,4, // following is for B slice
    4,2,4,2,
    4,2,4,2,
    4,2,4,2,
    4,2,4,2,
    4,2,2};
  const unsigned char val_stepY[30]= {4,4,2,4,2,2,
    2,2,4,4,4,
    2,4,2,4,
    2,4,2,4,
    2,4,2,4,
    2,4,2,4,
    2,4,2};

  const unsigned char val_listuse[30][4]= {
    //following are P slice
    {1,1,1,1},//skip
    {1,1,1,1},//0
    {1,1,1,1},//1
    {1,1,1,1},//2
    {1,1,1,1},//3
    {1,1,1,1},//4
    // following are B slice
    {0,0,0,0},//skip
    {0,0,0,0},//direct
    {1,1,1,1},//1
    {2,2,2,2},//2
    {3,3,3,3},//3
    {1,1,1,1},//4
    {1,1,1,1},//5
    {2,2,2,2},//6
    {2,2,2,2},//7
    {1,1,2,2},//8
    {1,2,1,2},//9
    {2,2,1,1},//10
    {2,1,2,1},//11
    {1,1,3,3},//12
    {1,3,1,3},//13
    {2,2,3,3},//14
    {2,3,2,3},//15
    {3,3,1,1},//16
    {3,1,3,1},//17
    {3,3,2,2},//18
    {3,2,3,2},//19
    {3,3,3,3},//20
    {3,3,3,3},//21
    {0,0,0,0},//22
  };

  const unsigned char val_predir[30][4]=  {
    //following are P slice
    {0,0,0,0},//skip
    {0,0,0,0},//0
    {2,2,1,1},//1
    {1,3,1,3},//2
    {0,0,0,0},//3
    {0,0,0,0},//4
    //following are B slice
    {0,0,0,0},//skip
    {0,0,0,0},//direct
    {0,0,0,0},//1
    {0,0,0,0},//2
    {0,0,0,0},//3
    {2,2,1,1},//4
    {1,3,1,3},//5
    {2,2,1,1},//6
    {1,3,1,3},//7
    {2,2,1,1},//8
    {1,3,1,3},//9
    {2,2,1,1},//10
    {1,3,1,3},//11
    {2,2,1,1},//12
    {1,3,1,3},//13
    {2,2,1,1},//14
    {1,3,1,3},//15
    {2,2,1,1},//16
    {1,3,1,3},//17
    {2,2,1,1},//18
    {1,3,1,3},//19
    {2,2,1,1},//20
    {1,3,1,3},//21
    {0,0,0,0},//22
  };



  const unsigned char val_sublistuse[17]={ 1,1,1,1,
    0,1,2,3,1,1,2,2,3,3,1,2,3};


  const unsigned char val_subX[17]={
    2,2,1,1,// P slice
    2,2,2,2,
    2,1,2,1,
    2,1,1,1,1
  };
  const unsigned char val_subY[17]={
    2,1,2,1,// P slice
    2,2,2,2,
    1,2,1,2,
    1,2,1,1,1
  };

  listuse[0][0]=val_listuse[MbType][0];
  listuse[1][0]=val_listuse[MbType][1];
  listuse[0][1]=val_listuse[MbType][2];
  listuse[1][1]=val_listuse[MbType][3];
  stepx=val_stepX[MbType];
  stepy=val_stepY[MbType];
  substepx[0][0]=stepx;
  substepy[0][0]=stepy;
  substepx[0][1]=stepx;
  substepy[0][1]=stepy;
  substepx[1][0]=stepx;
  substepy[1][0]=stepy;
  substepx[1][1]=stepx;
  substepy[1][1]=stepy;
  preddir[0][0]=val_predir[MbType][0];
  preddir[1][0]=val_predir[MbType][1];
  preddir[0][1]=val_predir[MbType][2];
  preddir[1][1]=val_predir[MbType][3];

  if(MbType==5 || MbType==29)
  {

    for(i=0;i<2;i++)
      for(j=0;j<2;j++)
      {
        subtype=u_e(nalu)+slice_type*4;
        substepx[j][i]=val_subX[subtype];
        substepy[j][i]=val_subY[subtype];
        listuse[j][i]=val_sublistuse[subtype];
      }
  }

  skipflag= (MbType==6 || MbType==7)?0:skipflag;

  if(MbType==0 || MbType==5  )
  {

    for(i=0;i<2;i++)
      for(j=0;j<2;j++)
      {
        refidx0[i][j] = 0;
        refidx1[i][j] = -1;
        globref0[startblkx/2+j][startblky/2+i]=0;
        globref1[startblkx/2+j][startblky/2+i]=-1;
      }
  }
  else
  {
    if(num_ref_idx_active0>0 )
    {
      for(y=0;y<2;y+= (stepy/2) )
        for(x=0;x<2;x+= (stepx/2) )
        {
          if(listuse[x][y]== 0)
          {
            dflag= find_directzeroflag
              (
               globref0,
               globref1,
               tmpref01,
               startblkx,
               startblky
              );
            refidx0[x][y]=globref0[startblkx/2+x][startblky/2+y]=tmpref01[0];
            refidx1[x][y]=globref1[startblkx/2+x][startblky/2+y]=tmpref01[1];


          }
          else if(listuse[x][y] & 0x01)
          {
            refidx=t_e(num_ref_idx_active0,nalu);
#if _N_HLS_
            fprintf(trace_bit,"refIdxL0[i][j] %d %d \n" ,refidx, num_ref_idx_active0);
#endif // _N_HLS_
            for(i=0;i<stepy/2;i++)
              for(j=0;j<stepx/2;j++)
              {
                refidx0[x+j][y+i]=refidx;
                globref0[startblkx/2+x+j][startblky/2+y+i]=refidx;
              }
          }
          else
          {
            for(i=0;i<stepy/2;i++)
              for(j=0;j<stepx/2;j++)
              {
                refidx0[x+j][y+i]=-1;
                globref0[startblkx/2+x+j][startblky/2+y+i]=-1;
              }
          }
        }
    }
    else
    {
      for(i=0;i<2;i++)
        for(j=0;j<2;j++)
        {
          refidx0[j][i]=0;
          globref0[startblkx/2+j][startblky/2+i]=0;
        }
    }
    if(num_ref_idx_active1>0  )
    {
      for(y=0;y<2;y+= (stepy/2) )
        for(x=0;x<2;x+= (stepx/2) )
        {

          if(listuse[x][y]== 0)
          {
            //donotihing
          }
          else if( (listuse[x][y] & 0x02) !=0)
          {
            refidx=t_e(num_ref_idx_active1,nalu);
#if _N_HLS_
            fprintf(trace_bit,"refIdxL1[i][j] %d %d\n" , refidx, num_ref_idx_active1);
#endif // _N_HLS_
            for(i=0;i<stepy/2;i++)
              for(j=0;j<stepx/2;j++)
              {
                refidx1[x+j][y+i]=refidx;
                globref1[startblkx/2+x+j][startblky/2+y+i]=refidx;
#if _N_HLS_
                fprintf(trace_bit,"refIdx1[%d][%d] %d\n" ,x+j, y+i, refidx);
#endif // _N_HLS_
              }
          }
          else
          {
            for(i=0;i<stepy/2;i++)
              for(j=0;j<stepx/2;j++)
              {
                refidx1[x+j][y+i]=-1;
                globref1[startblkx/2+x+j][startblky/2+y+i]=-1;
              }
          }
        }
    }
    else
    {

      for(i=0;i<2;i++)
        for(j=0;j<2;j++)
        {
          refidx1[j][i]=0;
          globref1[startblkx/2+j][startblky/2+i]=0;
        }
    }
  }
  unsigned char stpw;
  unsigned char stph;

  for(y=0;y<4;y+=stepy)
    for(x=0;x<4;x+=stepx)
    {
      if(listuse[x>>1][y>>1]==0)
      {
        findmv( globref0,globmvd0,startblkx, startblky,0,4,0,0,refidx0[x/2][y/2],foundmv0,0,0);
        findmv( globref1,globmvd1,startblkx, startblky,0,4,0,0,refidx1[x/2][y/2],foundmv1,0,0);

        for(i=0;i<2;i++)
          for(j=0;j<2;j++)
          {
            tpmmv[0]=foundmv0[0];
            tpmmv[1]=foundmv0[1];
            tpmmv1[0]=foundmv1[0];
            tpmmv1[1]=foundmv1[1];

            find_directmv_flag(dflag,refcol,mvcol,x+i,y+j,tpmmv1,tpmmv,refidx0[x/2][y/2],refidx1[x/2][y/2]);
            mvd0[x+i][y+j][0]=globmvd0[startblkx+x+i][startblky+y+j][0]=tpmmv[0];
            mvd0[x+i][y+j][1]=globmvd0[startblkx+x+i][startblky+y+j][1]=tpmmv[1];
            mvd1[x+i][y+j][0]=globmvd1[startblkx+x+i][startblky+y+j][0]=tpmmv1[0];
            mvd1[x+i][y+j][1]=globmvd1[startblkx+x+i][startblky+y+j][1]=tpmmv1[1];
          }
      }
      else if( listuse[x>>1][y>>1] & 0x01 )
      {
        stpw=substepx[x>>1][y>>1];
        stph=substepy[x>>1][y>>1];

        for(suby=0;suby<stepy;suby+=stph)
          for(subx=0;subx<stepx;subx+=stpw)
          {
            //calculate mvbase
            if(skipflag!=1  )
            {
              mvdx=s_e(nalu);
              mvdy=s_e(nalu);
#if _N_HLS_
              fprintf(trace_bit," curMB->mvd_l0[%d][%d][0] %d\n" ,startblkx+x+subx,startblky+y+suby,mvdx);
#endif // _N_HLS_
#if _N_HLS_
              fprintf(trace_bit," curMB->mvd_l0[%d][%d][1] %d\n" ,startblkx+x+subx,startblky+y+suby,mvdy);

#endif // _N_HLS_
              // calculate and write x
            }
            else
            {
              mvdx=0;
              mvdy=0;
            }

            findmv
              (
               globref0,
               globmvd0,
               startblkx,
               startblky,
               XYTOK(x+subx,y+suby),
               stpw,
               x+subx,
               y+suby,
               refidx0[x/2][y/2],
               tpmmv,
               preddir[x/2][y/2],
               skipflag
              );

            for(i=0;i<stpw;i++)
              for(j=0;j<stph;j++)
              {
                mvd0[x+subx+i][y+suby+j][0]=globmvd0[startblkx+x+subx+i][startblky+y+suby+j][0]=mvdx+tpmmv[0];
                mvd0[x+subx+i][y+suby+j][1]=globmvd0[startblkx+x+subx+i][startblky+y+suby+j][1]=mvdy+tpmmv[1];
              }
          }
      }
    }

  for(y=0;y<4;y+=stepy)
    for(x=0;x<4;x+=stepx)
    {
      if(listuse[x>>1][y>>1]==0)
      {
        // this part left
      }
      else if( listuse[x>>1][y>>1] & 0x02 )
      {
        stpw=substepx[x>>1][y>>1];
        stph=substepy[x>>1][y>>1];

        for(suby=0;suby<stepy;suby+=stph)
          for(subx=0;subx<stepx;subx+=stpw)
          {
            //calculate mvbase
            if(skipflag!=1  )
            {
              mvdx=s_e(nalu);
              mvdy=s_e(nalu);
#if _N_HLS_
              fprintf(trace_bit," curMB->mvd_l0[%d][%d][0] %d\n" ,startblkx+x+subx,startblky+y+suby,mvdx);
#endif // _N_HLS_
#if _N_HLS_
              fprintf(trace_bit," curMB->mvd_l0[%d][%d][1] %d\n" ,startblkx+x+subx,startblky+y+suby,mvdy);

#endif // _N_HLS_
              // calculate and write x
            }
            else
            {
              mvdx=0;
              mvdy=0;
            }

            findmv
              (
               globref1,
               globmvd1,
               startblkx,
               startblky,
               XYTOK(x+subx,y+suby),
               stpw,
               x+subx,
               y+suby,
               refidx1[x/2][y/2],
               tpmmv,
               preddir[x/2][y/2],
               skipflag
              );

            for(i=0;i<stpw;i++)
              for(j=0;j<stph;j++)
              {
                mvd1[x+subx+i][y+suby+j][0]=globmvd1[startblkx+x+subx+i][startblky+y+suby+j][0]=mvdx+tpmmv[0];
                mvd1[x+subx+i][y+suby+j][1]=globmvd1[startblkx+x+subx+i][startblky+y+suby+j][1]=mvdy+tpmmv[1];
              }
            // calculate and write x
          }

      }
    }
}

// Content of called function

#define FrameHeightInMbs 30


#define PicWidthInMBs 40

char find_directzeroflag
(
 char refidx0[PicWidthInMBs*2][FrameHeightInMbs*2],
 char refidx1[PicWidthInMBs*2][FrameHeightInMbs*2],
 char ref01[2],
 int startblkx,
 int startblky
 )
{
  int idxC[2];
  char refA0, refA1;
  char refB0, refB1;
  char refC0, refC1;

  if( (startblkx-1)>=0)
  {
    refA0=refidx0[ (startblkx-1) /2][ (startblky)/2];
    refA1=refidx1[ (startblkx-1) /2][ (startblky)/2];
  }
  else
  {
    refA0=-1;
    refA1=-1;
  }

  if( (startblky-1)>=0)
  {
    refB0=refidx0[ (startblkx)/2][ (startblky-1)/2];
    refB1=refidx1[ (startblkx)/2][ (startblky-1)/2];
  }
  else
  {
    refB0=-1;
    refB1=-1;
  }



  findCidx(startblkx,startblky,0,1,0,0,idxC);

  if(idxC[0] >=0 && idxC[1]>=0 )
  {
    refC0=refidx0[idxC[0]/2][idxC[1]/2];
    refC1=refidx1[idxC[0]/2][idxC[1]/2];
  }
  else
  {
    refC0=-1;
    refC1=-1;
  }

  ref01[0]=minpositive(refA0,minpositive(refB0,refC0));
  ref01[1]=minpositive(refA1,minpositive(refB1,refC1));

  if(ref01[0]<0 && ref01[1]<0)
  {
    ref01[0]=0;
    ref01[1]=0;
    return 1;
  }
  else
  {
    return 0;
  }
}

// Content of called function
int minpositive(int x, int y)
{
  if(x>=0 && y>=0)
  {
    if(x>y)
      return y;
    else
      return x;
  }
  else
  {
    if(x<y)
      return y;
    else
      return x;
  }
}

// Content of called function
void find_directmv_flag
(
 unsigned char dflag,
 char refcol[2][2],
 int mvcol[4][4][2],
 int blkx,
 int blky,
 int mv1[2],
 int mv0[2],
 char refidx0,
 char refidx1
 )
{
  unsigned char colzero;
  if(refcol[blkx/2][blky/2]==0 && mvcol[blkx][blky][0]<=1 && mvcol[blkx][blky][0]>=-1 && mvcol[blkx][blky][1]<=1 && mvcol[blkx][blky][1]>=-1)
    colzero=1;
  else
    colzero=0;

  if(dflag || refidx0<0 || (refidx0==0 && colzero) )
  {

    mv0[0]=0;
    mv0[1]=0;
  }
  if(dflag || refidx1<0 || (refidx1==0 && colzero) )
  {

    mv1[0]=0;
    mv1[1]=0;
  }
}

// Content of called function

#define FrameHeightInMbs 30


#define PicWidthInMBs 40

void IntraInfo(
    NALU_t* nalu,
    char pred_mode[PicWidthInMBs*4][FrameHeightInMbs*4],
    char refidx0[PicWidthInMBs*2][FrameHeightInMbs*2],
    char refidx1[PicWidthInMBs*2][FrameHeightInMbs*2],
    char tmpintramode[16],
    unsigned char constrained_intra_pred_flag,
    int startx,
    int starty)
{
  int x;
  int y;
  int xoff;
  int yoff;
  int k;
  int modetmp;
  char predmodeA,predmodeB;
  unsigned char previntramodeflag;

  for(k=0;k<16;k++)
  {

    x=KTOX(k);
    y=KTOY(k);

    xoff=startx+x;
    yoff=starty+y;

    refidx0[xoff/2][yoff/2]=-1;
    refidx1[xoff/2][yoff/2]=-1;

    predmodeA=2;
    predmodeB=2;


    if(xoff>0 && constrained_intra_pred_flag==0  && yoff>0 )
    {
      predmodeA=pred_mode[xoff-1][yoff];
      predmodeB=pred_mode[xoff][yoff-1];
    }

    modetmp=MIN(predmodeA,predmodeB);


    previntramodeflag=u_1(nalu);
#if _N_HLS_
    fprintf(trace_bit,"%s %*d\n","prev_intra_pred_mode_flag",50-strlen("prev_intra_pred_mode_flag"),previntramodeflag);
#endif // _N_HLS_

    if(previntramodeflag)
    {

      tmpintramode[k]=modetmp;

    }
    else
    {
      tmpintramode[k]=u_n(3,nalu);
#if _N_HLS_
      fprintf(trace_bit,"%s %*d\n","rem_intra_pred_mode",50-strlen("rem_intra_pred_mode"),tmpintramode[k]);
#endif // _N_HLS_

      if(tmpintramode[k]>=modetmp)
        tmpintramode[k]=tmpintramode[k]+1;
    }
    pred_mode[xoff][yoff]=tmpintramode[k];
  }
}

// Content of called function
#define FrameHeightInSampleL	(FrameHeightInMbs*16)

#define PicWidthInSamplesL (PicWidthInMBs*16)

void predict_intra16x16_luma_NonField(unsigned char predL[16][4][4], unsigned char Sluma[PicWidthInSamplesL][FrameHeightInSampleL] , unsigned char predmode, unsigned char avaiMode, unsigned int startx, unsigned int starty)
{
#pragma HLS ARRAY_PARTITION variable=predL complete dim=2
#pragma HLS ARRAY_PARTITION variable=predL complete dim=3

  int i,j,k;
  int x,y;
  unsigned char v[16];
#pragma HLS ARRAY_PARTITION variable=v dim=1 complete
  unsigned char h[16];
#pragma HLS ARRAY_PARTITION variable=h dim=1 complete

  unsigned char X;

  if(avaiMode/2)
    for(i=0;i<16;i++)
      #pragma HLS UNROLL
      h[i]=Sluma[startx-1][starty+i];
  else
    for(i=0;i<16;i++) 
      #pragma HLS UNROLL
      h[i]=128;


  if(avaiMode%2)
    for(i=0;i<16;i++) 
      #pragma HLS UNROLL
      v[i]=Sluma[startx+i][starty-1];
  else
    for(i=0;i<16;i++)  
      #pragma HLS UNROLL
      v[i]=128;


  if(avaiMode==3)
  {
    X=Sluma[startx-1][starty-1];
  }
  else
    X=128;

  switch(predmode)
  {
    case 0:
      {
        for(k=0;k<16;k++)
          #pragma HLS PIPELINE
          for(i=0;i<4;i++)
            for(j=0;j<4;j++)
            {
              x=KTOX(k)*4+i;

              predL[k][i][j]=v[x];
            }
      }
      break;
    case 1:
      {
        for(k=0;k<16;k++)
          #pragma HLS PIPELINE
          for(i=0;i<4;i++)
            for(j=0;j<4;j++)
            {
              y=KTOY(k)*4+j;

              predL[k][i][j]=h[y];
            }
      }
      break;
    case 2:
      {
        int sumx=0;
        int sumy=0;

        if(avaiMode%2)
          for(x=0;x<16;x++)
          {
            #pragma HLS UNROLL
            sumx=sumx+v[x];
          }

        if(avaiMode/2)
          for(y=0;y<16;y++)
          {
            #pragma HLS UNROLL
            sumy=sumy+h[y];
          }

        int temp;
        switch (avaiMode)
        {
          case 3:
            temp = (sumx+sumy+16)>>5;
            break;
          case 2:
            temp = (sumy+8)>>4;
            break;
          case 1:
            temp = (sumx+8)>>4;
            break;
          case 0:
            temp = 128;
        }

        for(k=0;k<16;k++)
          #pragma HLS PIPELINE
          for(i=0;i<4;i++)
            for(j=0;j<4;j++)
            {
              predL[k][i][j]=temp;
            }
      }
      break;
    default:
      {
        int H;
        int V;
        int a;
        int tmp;

        H=v[8]-v[6]+2*(v[9]-v[5])+3*(v[10]-v[4])+4*(v[11]-v[3])+5*(v[12]-v[2])+6*(v[13]-v[1])+7*(v[14]-v[0])+8*(v[15]-X);
        V=h[8]-h[6]+2*(h[9]-h[5])+3*(h[10]-h[4])+4*(h[11]-h[3])+5*(h[12]-h[2])+6*(h[13]-h[1])+7*(h[14]-h[0])+8*(h[15]-X);
        H=(5*H+32)>>6;
        V=(5*V+32)>>6;
        a=16*(v[15]+h[15]);

        for(k=0;k<16;k++)
          #pragma HLS PIPELINE
          for(i=0;i<4;i++)
            for(j=0;j<4;j++)
            {
              x=KTOX(k)*4+i;
              y=KTOY(k)*4+j;
              tmp=(a+H*(x-7)+V*(y-7)+16)>>5;

              if(tmp<0)
                predL[k][i][j]=0;
              else if(tmp>255)
                predL[k][i][j]=255;
              else
                predL[k][i][j]=tmp;
            }
      }
      break;
  }
}

// Content of called function
#define FrameHeightInSampleC	(FrameHeightInMbs*MbHeight_C)

#define PicWidthInSamplesC	(PicWidthInMBs*MbWidth_C)

void prediction_Chroma(
    unsigned char predC[4][4][4],
    unsigned char SChroma[PicWidthInSamplesC][FrameHeightInSampleC],
    unsigned char avaimode,
    int startx,
    int starty,
    unsigned char pred_mod)
{
#pragma HLS ARRAY_PARTITION variable=predC complete dim=2
#pragma HLS ARRAY_PARTITION variable=predC complete dim=3


  unsigned char v[8];
#pragma HLS ARRAY_PARTITION variable=v complete dim=1
  unsigned char h[8];
#pragma HLS ARRAY_PARTITION variable=h complete dim=1

  unsigned char x,y;
  unsigned char X;
  int H,V;
  int i,j,k;
  int a;
  int temp;

  if(avaimode/2)
    for(i=0;i<8;i++) 
      #pragma HLS UNROLL
      h[i]=SChroma[startx-1][starty+i];
  else
    for(i=0;i<8;i++) 
      #pragma HLS UNROLL
      h[i]=128;

  if(avaimode%2)
    for(i=0;i<8;i++) 
      #pragma HLS UNROLL
      v[i]=SChroma[startx+i][starty-1];
  else
    for(i=0;i<8;i++)  
      #pragma HLS UNROLL
      v[i]=128;

  if(avaimode==3)
  {
    X=SChroma[startx-1][starty-1];
  }
  else
    X=128;

  switch(pred_mod)
  {
    case 0:
      //prediction_Chroma_DC(predC,SChroma,avaimode,startx,starty);
      {
        int js1,js2,js3,js0;
        js0=0;
        js1=0;
        js2=0;
        js3=0;

        if(avaimode%2)
        {
          for(x=0;x<4;x++)
          {
            #pragma HLS UNROLL
            js0+=v[x];
            js1+=v[x+4];
          }
        }
        if(avaimode/2)
        {
          for(y=0;y<4;y++)
          {
            #pragma HLS UNROLL
            js2+=h[y];
            js3+=h[y+4];
          }
        }

        int temp[2][2];

        switch(avaimode)
        {
          case 0:
            temp[0][0] = 128;
            temp[0][1] = 128;
            temp[1][0] = 128;
            temp[1][1] = 128;
            break;
          case 1:
            temp[0][0] = (js0 + 2) >> 2;
            temp[0][1] = (js1 + 2) >> 2;
            temp[1][0] = (js0 + 2) >> 2;
            temp[1][1]=  (js1 + 2) >> 2;
            break;
          case 2:
            temp[0][0] = (js2 + 2) >> 2;
            temp[0][1] = (js2 + 2) >> 2;
            temp[1][0] = (js3 + 2) >> 2;
            temp[1][1] = (js3 + 2) >> 2;
            break;
          default:
            temp[0][0] = (js2 + js0 + 4) >> 3;
            temp[0][1] = (js1 + 2) >> 2;
            temp[1][0] = (js3 + 2) >> 2;
            temp[1][1] = (js1 + js3 + 4) >> 3;
            break;
        }

        int i,j;

        for(i=0;i<2;i++)
          #pragma HLS PIPELINE
          for(j=0;j<2;j++)
            for(x=0;x<4;x++)
              for(y=0;y<4;y++)
              {
                predC[j+i*2][x][y]=temp[i][j];
              }
      }
      break;
    case 1:
      // prediction_Chroma_Horizontal(predC,SChroma,startx,starty);


      for(k=0;k<4;k++)
        #pragma HLS PIPELINE
        for(i=0;i<4;i++)
          for(j=0;j<4;j++)
          {
            predC[k][i][j]=h[ (k/2)*4+j];
          }
      break;
    case 2:
      //prediction_Chroma_Vertical(predC,SChroma,startx,starty);
      for(k=0;k<4;k++)
        #pragma HLS PIPELINE
        for(i=0;i<4;i++)
          for(j=0;j<4;j++)
          {
            predC[k][i][j]=v[ (k%2)*4+i];
          }
      break;
    default:
      //prediction_Chroma_Plane(predC,SChroma,startx,starty);
      //flattem

      H=v[4]-v[2] +2*(v[5]-v[1])+3*(v[6]-v[0])+4*(v[7]-X);
      V=h[4]-h[2] +2*(h[5]-h[1])+3*(h[6]-h[0])+4*(h[7]-X);
      H=(17*H+16)>>5;
      V=(17*V+16)>>5;
      a=16*(v[7]+h[7]);

      for(k=0;k<4;k++)
        #pragma HLS PIPELINE
        for(i=0;i<4;i++)
          for(j=0;j<4;j++)
          {
            temp= (a+H*( (k%2)*4+i-3)+V*((k/2)*4+j-3)+16)>>5;
            if(temp<0)
              predC[k][i][j]=0;
            else if(temp>255)
              predC[k][i][j]=255;
            else
              predC[k][i][j]=temp;
          }
      break;
  }
}

// Content of called function

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

// Content of called function

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