uint18_t get_data26(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[70 ]; break;
    case 1: a = aa[89 ]; break;
    case 2: a = aa[115]; break;
    case 3: a = aa[127]; break;
    case 4: a = aa[142]; break;
    case 5: a = aa[272]; break;
    case 6: a = aa[348]; break;
    case 7: a = aa[370]; break;
    case 8: a = aa[379]; break;
    case 9: a = aa[430]; break;
    case 10:a = aa[461]; break;
    case 11:a = aa[485]; break;
    case 12:a = aa[513]; break;
    case 13:a = aa[541]; break;
    case 14:a = aa[550]; break;
    case 15:a = aa[583]; break;
    case 16:a = aa[607]; break;
    default: a = 0; break;
  }; 
  return a;
}