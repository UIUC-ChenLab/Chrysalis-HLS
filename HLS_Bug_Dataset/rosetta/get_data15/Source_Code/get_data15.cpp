uint18_t get_data15(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[8  ]; break;
    case 1: a = aa[25 ]; break;
    case 2: a = aa[30 ]; break;
    case 3: a = aa[57 ]; break;
    case 4: a = aa[120]; break;
    case 5: a = aa[123]; break;
    case 6: a = aa[169]; break;
    case 7: a = aa[192]; break;
    case 8: a = aa[217]; break;
    case 9: a = aa[241]; break;
    case 10:a = aa[271]; break;
    case 11:a = aa[274]; break;
    case 12:a = aa[285]; break;
    case 13:a = aa[306]; break;
    case 14:a = aa[327]; break;
    case 15:a = aa[368]; break;
    case 16:a = aa[403]; break;
    case 17:a = aa[434]; break;
    case 18:a = aa[474]; break;
    case 19:a = aa[476]; break;
    case 20:a = aa[504]; break;
    case 21:a = aa[538]; break;
    case 22:a = aa[563]; break;
    case 23:a = aa[568]; break;
    case 24:a = aa[596]; break;
    default: a = 0; break;
  }; 
  return a;
}