uint18_t get_data27(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[36 ]; break;
    case 1: a = aa[91 ]; break;
    case 2: a = aa[98 ]; break;
    case 3: a = aa[107]; break;
    case 4: a = aa[176]; break;
    case 5: a = aa[202]; break;
    case 6: a = aa[278]; break;
    case 7: a = aa[467]; break;
    case 8: a = aa[482]; break;
    case 9: a = aa[546]; break;
    case 10:a = aa[556]; break;
    case 11:a = aa[569]; break;
    case 12:a = aa[592]; break;
    case 13:a = aa[620]; break;
    default: a = 0; break;
  }; return a;
}