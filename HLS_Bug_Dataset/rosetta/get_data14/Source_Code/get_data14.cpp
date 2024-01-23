uint18_t get_data14(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[3  ]; break;
    case 1: a = aa[10 ]; break;
    case 2: a = aa[17 ]; break;
    case 3: a = aa[35 ]; break;
    case 4: a = aa[66 ]; break;
    case 5: a = aa[99 ]; break;
    case 6: a = aa[152]; break;
    case 7: a = aa[178]; break;
    case 8: a = aa[248]; break;
    case 9: a = aa[259]; break;
    case 10:a = aa[270]; break;
    case 11:a = aa[290]; break;
    case 12:a = aa[321]; break;
    case 13:a = aa[336]; break;
    case 14:a = aa[337]; break;
    case 15:a = aa[361]; break;
    case 16:a = aa[382]; break;
    case 17:a = aa[393]; break;
    case 18:a = aa[416]; break;
    case 19:a = aa[473]; break;
    case 20:a = aa[502]; break;
    case 21:a = aa[545]; break;
    case 22:a = aa[600]; break;
    default: a = 0; break;
  }; 
  return a;
}