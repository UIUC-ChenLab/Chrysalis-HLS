void gradient_weight_x(gradient_t y_filt[MAX_HEIGHT][MAX_WIDTH],
                       gradient_t filt_grad[MAX_HEIGHT][MAX_WIDTH])
{
  hls::Window<1,7,gradient_t> buf;
  const pixel_t GRAD_FILTER[] = {0.0755, 0.133, 0.1869, 0.2903, 0.1869, 0.133, 0.0755};
  GRAD_WEIGHT_X_OUTER: for(int r=0; r<MAX_HEIGHT; r++)
  {
    GRAD_WEIGHT_X_INNER: for(int c=0; c<MAX_WIDTH+3; c++)
    {
      #pragma HLS pipeline II=1
      buf.shift_pixels_left();
      gradient_t tmp;
      if(c<MAX_WIDTH)
      {
        tmp = y_filt[r][c];
      }
      else
      {
        tmp.x = 0;
        tmp.y = 0;
        tmp.z = 0;  
      }
      buf.insert_pixel(tmp,0,6);

      gradient_t acc;
      acc.x = 0;
      acc.y = 0;
      acc.z = 0;
      if(c >= 6 && c<MAX_WIDTH) 
      {
        GRAD_WEIGHT_X_ACC: for(int i=0; i<7; i++)
        {
          acc.x += buf.getval(0,i).x*GRAD_FILTER[i];
          acc.y += buf.getval(0,i).y*GRAD_FILTER[i];
          acc.z += buf.getval(0,i).z*GRAD_FILTER[i];
        }
        filt_grad[r][c-3] = acc;
      }
      else if(c>=3)
      {
        filt_grad[r][c-3] = acc;
      }
    }
  }
}