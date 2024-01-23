uint18_t get_data3(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[41 ]; break;
    case 1: a = aa[56 ]; break;
    case 2: a = aa[79 ]; break;
    case 3: a = aa[96 ]; break;
    case 4: a = aa[109]; break;
    case 5: a = aa[141]; break;
    case 6: a = aa[155]; break;
    case 7: a = aa[201]; break;
    case 8: a = aa[249]; break;
    case 9: a = aa[263]; break;
    case 10:a = aa[293]; break;
    case 11:a = aa[322]; break;
    case 12:a = aa[383]; break;
    case 13:a = aa[394]; break;
    case 14:a = aa[408]; break;
    case 15:a = aa[415]; break;
    case 16:a = aa[428]; break;
    case 17:a = aa[445]; break;
    case 18:a = aa[459]; break;
    case 19:a = aa[479]; break;
    case 20:a = aa[532]; break;
    case 21:a = aa[564]; break;
    case 22:a = aa[575]; break;
    case 23:a = aa[598]; break;
    case 24:a = aa[611]; break;
    default: a = 0; break;
  };
  return a;
}