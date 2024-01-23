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