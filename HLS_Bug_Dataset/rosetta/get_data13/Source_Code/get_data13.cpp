uint18_t get_data13(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[1  ]; break;
    case 1: a = aa[27 ]; break;
    case 2: a = aa[59 ]; break;
    case 3: a = aa[87 ]; break;
    case 4: a = aa[118]; break;
    case 5: a = aa[131]; break;
    case 6: a = aa[167]; break;
    case 7: a = aa[175]; break;
    case 8: a = aa[247]; break;
    case 9: a = aa[319]; break;
    case 10:a = aa[334]; break;
    case 11:a = aa[335]; break;
    case 12:a = aa[371]; break;
    case 13:a = aa[387]; break;
    case 14:a = aa[395]; break;
    case 15:a = aa[414]; break;
    case 16:a = aa[442]; break;
    case 17:a = aa[501]; break;
    case 18:a = aa[544]; break;
    case 19:a = aa[548]; break;
    case 20:a = aa[565]; break;
    case 21:a = aa[603]; break;
    case 22:a = aa[604]; break;
    default: a = 0; break;
  }; 
  return a;
}