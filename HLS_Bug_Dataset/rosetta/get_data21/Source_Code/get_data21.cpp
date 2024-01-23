uint18_t get_data21(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[4  ]; break;
    case 1: a = aa[32 ]; break;
    case 2: a = aa[73 ]; break;
    case 3: a = aa[81 ]; break;
    case 4: a = aa[108]; break;
    case 5: a = aa[172]; break;
    case 6: a = aa[190]; break;
    case 7: a = aa[194]; break;
    case 8: a = aa[224]; break;
    case 9: a = aa[266]; break;
    case 10:a = aa[318]; break;
    case 11:a = aa[338]; break;
    case 12:a = aa[360]; break;
    case 13:a = aa[392]; break;
    case 14:a = aa[437]; break;
    case 15:a = aa[475]; break;
    case 16:a = aa[505]; break;
    case 17:a = aa[574]; break;
    case 18:a = aa[601]; break;
    default: a = 0; break;
  }; 
  return a;
}