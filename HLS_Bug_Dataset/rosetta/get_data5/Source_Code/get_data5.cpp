uint18_t get_data5(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[23 ]; break;
    case 1: a = aa[53 ]; break;
    case 2: a = aa[94 ]; break;
    case 3: a = aa[95 ]; break;
    case 4: a = aa[101]; break;
    case 5: a = aa[139]; break;
    case 6: a = aa[171]; break;
    case 7: a = aa[180]; break;
    case 8: a = aa[222]; break;
    case 9: a = aa[267]; break;
    case 10:a = aa[275]; break;
    case 11:a = aa[311]; break;
    case 12:a = aa[312]; break;
    case 13:a = aa[333]; break;
    case 14:a = aa[365]; break;
    case 15:a = aa[390]; break;
    case 16:a = aa[397]; break;
    case 17:a = aa[409]; break;
    case 18:a = aa[410]; break;
    case 19:a = aa[426]; break;
    case 20:a = aa[443]; break;
    case 21:a = aa[463]; break;
    case 22:a = aa[537]; break;
    case 23:a = aa[571]; break;
    case 24:a = aa[599]; break;
    case 25:a = aa[608]; break;
    default: a = 0; break;
  }; 
  return a;
}