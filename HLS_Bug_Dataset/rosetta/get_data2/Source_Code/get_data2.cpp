uint18_t get_data2(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[9  ]; break;
    case 1: a = aa[20 ]; break;
    case 2: a = aa[28 ]; break;
    case 3: a = aa[48 ]; break;
    case 4: a = aa[74 ]; break;
    case 5: a = aa[97 ]; break;
    case 6: a = aa[168]; break;
    case 7: a = aa[187]; break;
    case 8: a = aa[188]; break;
    case 9: a = aa[213]; break;
    case 10:a = aa[233]; break;
    case 11:a = aa[260]; break;
    case 12:a = aa[261]; break;
    case 13:a = aa[277]; break;
    case 14:a = aa[303]; break;
    case 15:a = aa[314]; break;
    case 16:a = aa[329]; break;
    case 17:a = aa[356]; break;
    case 18:a = aa[375]; break;
    case 19:a = aa[376]; break;
    case 20:a = aa[452]; break;
    case 21:a = aa[499]; break;
    case 22:a = aa[519]; break;
    case 23:a = aa[529]; break;
    case 24:a = aa[536]; break;
    case 25:a = aa[551]; break;
    case 26:a = aa[567]; break;
    case 27:a = aa[597]; break;
    case 28:a = aa[615]; break;
    default: a = 0; break;
  };
  return a;
}