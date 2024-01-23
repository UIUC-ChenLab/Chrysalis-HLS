uint18_t get_data7(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[38 ]; break;
    case 1: a = aa[82 ]; break;
    case 2: a = aa[93 ]; break;
    case 3: a = aa[135]; break;
    case 4: a = aa[159]; break;
    case 5: a = aa[195]; break;
    case 6: a = aa[212]; break;
    case 7: a = aa[237]; break;
    case 8: a = aa[238]; break;
    case 9: a = aa[258]; break;
    case 10:a = aa[269]; break;
    case 11:a = aa[283]; break;
    case 12:a = aa[310]; break;
    case 13:a = aa[328]; break;
    case 14:a = aa[355]; break;
    case 15:a = aa[421]; break;
    case 16:a = aa[427]; break;
    case 17:a = aa[465]; break;
    case 18:a = aa[523]; break;
    case 19:a = aa[547]; break;
    case 20:a = aa[557]; break;
    case 21:a = aa[570]; break;
    case 22:a = aa[593]; break;
    case 23:a = aa[606]; break;
    default: a = 0; break;
  }; 
  return a;
}