void tensor_weight_x(tensor_t tensor_y[MAX_HEIGHT][MAX_WIDTH],
                     tensor_t tensor[MAX_HEIGHT][MAX_WIDTH])
{
  hls::Window<1,3,tensor_t> buf;
  const pixel_t TENSOR_FILTER[] = {0.3243, 0.3513, 0.3243};
  //const float TENSOR_FILTER[] = {0.3243, 0.3513, 0.3243};
  TENSOR_WEIGHT_X_OUTER: for(int r=0; r<MAX_HEIGHT; r++)
  {
    TENSOR_WEIGHT_X_INNER: for(int c=0; c<MAX_WIDTH+1; c++)
    {
      #pragma HLS pipeline II=1
      buf.shift_pixels_left();
      tensor_t tmp;
      if(c<MAX_WIDTH)
      {
        tmp = tensor_y[r][c];
      }
      else
      {
        TENSOR_WEIGHT_X_TMP_INIT: for(int i=0; i<6; i++)
          tmp.val[i] = 0;
      }
      buf.insert_pixel(tmp,0,2);

      tensor_t acc;
      TENSOR_WEIGHT_X_ACC_INIT: for(int k =0; k<6; k++)
        acc.val[k] = 0;
      if (c >= 2 && c < MAX_WIDTH) 
      {
        TENSOR_WEIGHT_X_TMP_OUTER: for(int i=0; i<3; i++)
        {
          tmp = buf.getval(0,i);
          TENSOR_WEIGHT_X_TMP_INNER: for(int component=0; component<6; component++)
          {
            acc.val[component] += tmp.val[component]*TENSOR_FILTER[i];
          }
        }
      }
      if(c>=1)
      {
        tensor[r][c-1] = acc;
      }
    }
  }
}