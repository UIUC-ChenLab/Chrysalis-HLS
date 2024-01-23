void output_FB(bit8 frame_buffer[MAX_X][MAX_Y], bit32 output[NUM_FB])
{
  #pragma HLS INLINE
  bit32 out_FB = 0;
  OUTPUT_FB_ROW: for ( bit16 i = 0; i < MAX_X; i++)
  {
    #pragma HLS PIPELINE II=1
    OUTPUT_FB_COL: for ( bit16 j = 0; j < MAX_Y; j = j + 4)
    {
      out_FB( 7,  0) = frame_buffer[i][j + 0];
      out_FB(15,  8) = frame_buffer[i][j + 1];
      out_FB(23, 16) = frame_buffer[i][j + 2];
      out_FB(31, 24) = frame_buffer[i][j + 3];
      output[i * MAX_Y / 4 + j / 4] = out_FB;
    }
  }
}