uint18_t get_data8(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0:a = aa[0]; break;
    default: a = 0; break;
  }; 
  return a;
}