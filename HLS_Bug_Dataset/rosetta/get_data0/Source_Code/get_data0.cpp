uint18_t get_data0(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[19 ]; break;
    case 1: a = aa[29 ]; break;
    case 2: a = aa[52 ]; break;
    case 3: a = aa[100]; break;
    case 4: a = aa[132]; break;
    case 5: a = aa[161]; break;
    case 6: a = aa[193]; break;
    case 7: a = aa[220]; break;
    case 8: a = aa[239]; break;
    case 9: a = aa[253]; break;
    case 10:a = aa[284]; break;
    case 11:a = aa[309]; break;
    case 12:a = aa[362]; break;
    case 13:a = aa[385]; break;
    case 14:a = aa[396]; break;
    case 15:a = aa[447]; break;
    case 16:a = aa[448]; break;
    case 17:a = aa[449]; break;
    case 18:a = aa[451]; break;
    case 19:a = aa[466]; break;
    case 20:a = aa[492]; break;
    case 21:a = aa[531]; break;
    case 22:a = aa[562]; break;
    default: a = 0; break;
  }; 

  return a;
}