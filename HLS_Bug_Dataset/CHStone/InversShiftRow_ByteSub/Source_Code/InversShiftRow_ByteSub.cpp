void
InversShiftRow_ByteSub (int statemt[32], int nb)
{
  int temp;

  switch (nb)
    {
    case 4:
      temp = invSbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = invSbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = invSbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = invSbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = temp;

      temp = invSbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = invSbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = temp;
      temp = invSbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = invSbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = temp;

      temp = invSbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = invSbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = invSbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = invSbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = temp;

      statemt[0] = invSbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = invSbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = invSbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = invSbox[statemt[12] >> 4][statemt[12] & 0xf];
      break;
    case 6:
      temp = invSbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = invSbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = invSbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = invSbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = invSbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = invSbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = temp;

      temp = invSbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = invSbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = invSbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = temp;
      temp = invSbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = invSbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = invSbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = temp;

      temp = invSbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = invSbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = temp;
      temp = invSbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = invSbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = temp;
      temp = invSbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = invSbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = temp;

      statemt[0] = invSbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = invSbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = invSbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = invSbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = invSbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = invSbox[statemt[20] >> 4][statemt[20] & 0xf];
      break;
    case 8:
      temp = invSbox[statemt[29] >> 4][statemt[29] & 0xf];
      statemt[29] = invSbox[statemt[25] >> 4][statemt[25] & 0xf];
      statemt[25] = invSbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = invSbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = invSbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = invSbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = invSbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = invSbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = temp;

      temp = invSbox[statemt[30] >> 4][statemt[30] & 0xf];
      statemt[30] = invSbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = invSbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = invSbox[statemt[26] >> 4][statemt[26] & 0xf];
      statemt[26] = invSbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = invSbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = invSbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = invSbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = temp;

      temp = invSbox[statemt[31] >> 4][statemt[31] & 0xf];
      statemt[31] = invSbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = temp;
      temp = invSbox[statemt[27] >> 4][statemt[27] & 0xf];
      statemt[27] = invSbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = temp;
      temp = invSbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = invSbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = temp;
      temp = invSbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = invSbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = temp;

      statemt[0] = invSbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = invSbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = invSbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = invSbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = invSbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = invSbox[statemt[20] >> 4][statemt[20] & 0xf];
      statemt[24] = invSbox[statemt[24] >> 4][statemt[24] & 0xf];
      statemt[28] = invSbox[statemt[28] >> 4][statemt[28] & 0xf];
      break;
    }
}