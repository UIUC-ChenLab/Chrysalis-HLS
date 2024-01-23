uint18_t get_data11(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[22 ]; break;
    case 1: a = aa[45 ]; break;
    case 2: a = aa[102]; break;
    case 3: a = aa[136]; break;
    case 4: a = aa[137]; break;
    case 5: a = aa[156]; break;
    case 6: a = aa[157]; break;
    case 7: a = aa[183]; break;
    case 8: a = aa[184]; break;
    case 9: a = aa[210]; break;
    case 10:a = aa[221]; break;
    case 11:a = aa[235]; break;
    case 12:a = aa[291]; break;
    case 13:a = aa[324]; break;
    case 14:a = aa[344]; break;
    case 15:a = aa[353]; break;
    case 16:a = aa[377]; break;
    case 17:a = aa[398]; break;
    case 18:a = aa[417]; break;
    case 19:a = aa[418]; break;
    case 20:a = aa[454]; break;
    case 21:a = aa[511]; break;
    case 22:a = aa[524]; break;
    case 23:a = aa[540]; break;
    case 24:a = aa[559]; break;
    case 25:a = aa[584]; break;
    case 26:a = aa[613]; break;
    default: a = 0; break;
  }; 
  return a;
}