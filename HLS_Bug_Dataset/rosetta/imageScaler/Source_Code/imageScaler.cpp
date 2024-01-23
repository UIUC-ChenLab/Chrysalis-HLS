void imageScaler
(
  int src_height,
  int src_width,
  unsigned char Data[IMAGE_HEIGHT][IMAGE_WIDTH], 
  int dest_height,
  int dest_width,
  unsigned char IMG1_data[IMAGE_HEIGHT][IMAGE_WIDTH]
)
{
  int y;
  int j;
  int x;
  int i;
  unsigned char* t;
  unsigned char* p;
  int w1 = src_width;
  int h1 = src_height;
  int w2 = dest_width;
  int h2 = dest_height;

  int rat = 0;

  int x_ratio = (int)((w1<<16)/w2) +1; 
  int y_ratio = (int)((h1<<16)/h2) +1;


  imageScalerL1: for ( i = 0 ; i < IMAGE_HEIGHT ; i++ ){ 
    imageScalerL1_1: for (j=0;j < IMAGE_WIDTH ;j++){
      #pragma HLS pipeline
      if ( j < w2 && i < h2 ) 
        IMG1_data[i][j] =  Data[(i*y_ratio)>>16][(j*x_ratio)>>16];
       
    }
  }
}