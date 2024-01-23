uint18_t get_data12(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[5  ]; break;
    case 1: a = aa[39 ]; break;
    case 2: a = aa[54 ]; break;
    case 3: a = aa[61 ]; break;
    case 4: a = aa[75 ]; break;
    case 5: a = aa[76 ]; break;
    case 6: a = aa[106]; break;
    case 7: a = aa[140]; break;
    case 8: a = aa[165]; break;
    case 9: a = aa[209]; break;
    case 10:a = aa[245]; break;
    case 11:a = aa[246]; break;
    case 12:a = aa[316]; break;
    case 13:a = aa[347]; break;
    case 14:a = aa[412]; break;
    case 15:a = aa[413]; break;
    case 16:a = aa[444]; break;
    case 17:a = aa[464]; break;
    case 18:a = aa[490]; break;
    case 19:a = aa[530]; break;
    case 20:a = aa[534]; break;
    case 21:a = aa[535]; break;
    case 22:a = aa[560]; break;
    case 23:a = aa[586]; break;
    case 24:a = aa[618]; break;
    default: a = 0; break;
  }; 
  return a;
}