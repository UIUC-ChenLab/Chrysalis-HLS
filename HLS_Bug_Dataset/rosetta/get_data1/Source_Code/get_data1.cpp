uint18_t get_data1(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[7  ]; break;
    case 1: a = aa[18 ]; break;
    case 2: a = aa[65 ]; break;
    case 3: a = aa[72 ]; break;
    case 4: a = aa[148]; break;
    case 5: a = aa[149]; break;
    case 6: a = aa[153]; break;
    case 7: a = aa[164]; break;
    case 8: a = aa[191]; break;
    case 9: a = aa[208]; break;
    case 10:a = aa[229]; break;
    case 11:a = aa[240]; break;
    case 12:a = aa[251]; break;
    case 13:a = aa[256]; break;
    case 14:a = aa[280]; break;
    case 15:a = aa[384]; break;
    case 16:a = aa[450]; break;
    case 17:a = aa[478]; break;
    case 18:a = aa[506]; break;
    case 19:a = aa[517]; break;
    case 20:a = aa[527]; break;
    case 21:a = aa[542]; break;
    case 22:a = aa[554]; break;
    case 23:a = aa[573]; break;
    case 24:a = aa[576]; break;
    case 25:a = aa[621]; break;
    default: a = 0; break;
  }; 
  return a;
}