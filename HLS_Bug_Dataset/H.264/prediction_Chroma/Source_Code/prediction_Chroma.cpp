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