uint18_t get_data22(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[71 ]; break;
    case 1: a = aa[85 ]; break;
    case 2: a = aa[92 ]; break;
    case 3: a = aa[124]; break;
    case 4: a = aa[133]; break;
    case 5: a = aa[143]; break;
    case 6: a = aa[166]; break;
    case 7: a = aa[211]; break;
    case 8: a = aa[225]; break;
    case 9: a = aa[304]; break;
    case 10:a = aa[305]; break;
    case 11:a = aa[351]; break;
    case 12:a = aa[352]; break;
    case 13:a = aa[407]; break;
    case 14:a = aa[423]; break;
    case 15:a = aa[431]; break;
    case 16:a = aa[472]; break;
    case 17:a = aa[495]; break;
    case 18:a = aa[515]; break;
    case 19:a = aa[549]; break;
    case 20:a = aa[553]; break;
    case 21:a = aa[558]; break;
    case 22:a = aa[588]; break;
    case 23:a = aa[614]; break;
    default: a = 0; break;
  }; 
  return a;
}