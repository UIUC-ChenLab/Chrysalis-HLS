uint18_t get_data24(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[31 ]; break;
    case 1: a = aa[84 ]; break;
    case 2: a = aa[113]; break;
    case 3: a = aa[116]; break;
    case 4: a = aa[129]; break;
    case 5: a = aa[158]; break;
    case 6: a = aa[182]; break;
    case 7: a = aa[227]; break;
    case 8: a = aa[276]; break;
    case 9: a = aa[380]; break;
    case 10:a = aa[404]; break;
    case 11:a = aa[460]; break;
    case 12:a = aa[470]; break;
    case 13:a = aa[493]; break;
    case 14:a = aa[494]; break;
    case 15:a = aa[503]; break;
    case 16:a = aa[514]; break;
    case 17:a = aa[566]; break;
    case 18:a = aa[580]; break;
    case 19:a = aa[602]; break;
    case 20:a = aa[617]; break;
    default: a = 0; break;
  }; 
  return a;
}