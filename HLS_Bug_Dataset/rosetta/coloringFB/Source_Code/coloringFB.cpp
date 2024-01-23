void coloringFB(bit16 counter,  bit16 size_pixels, Pixel pixels[], bit8 frame_buffer[MAX_X][MAX_Y])
{
  #pragma HLS INLINE off

  if ( counter == 0 )
  {
    // initilize the framebuffer for a new image
    COLORING_FB_INIT_ROW: for ( bit16 i = 0; i < MAX_X; i++)
    {
      #pragma HLS PIPELINE II=1
      COLORING_FB_INIT_COL: for ( bit16 j = 0; j < MAX_Y; j++)
        frame_buffer[i][j] = 0;
    }
  }

  // update the framebuffer
  COLORING_FB: for ( bit16 i = 0; i < size_pixels; i++)
  {
    #pragma HLS PIPELINE II=1
    frame_buffer[ pixels[i].x ][ pixels[i].y ] = pixels[i].color;
  }

}