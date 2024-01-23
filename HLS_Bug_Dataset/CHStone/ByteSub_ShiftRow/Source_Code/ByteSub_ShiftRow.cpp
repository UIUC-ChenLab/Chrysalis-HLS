void
ByteSub_ShiftRow (int statemt[32], int nb)
{
  int temp;

  switch (nb)
    {
    case 4:
      temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = temp;

      temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = temp;
      temp = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = temp;

      temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = temp;

      statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      break;
    case 6:
      temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = Sbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = Sbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = temp;

      temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = Sbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = temp;
      temp = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = Sbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = temp;

      temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = temp;
      temp = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = Sbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = temp;
      temp = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = Sbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = temp;

      statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = Sbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = Sbox[statemt[20] >> 4][statemt[20] & 0xf];
      break;
    case 8:
      temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = Sbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = Sbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = Sbox[statemt[25] >> 4][statemt[25] & 0xf];
      statemt[25] = Sbox[statemt[29] >> 4][statemt[29] & 0xf];
      statemt[29] = temp;

      temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = Sbox[statemt[26] >> 4][statemt[26] & 0xf];
      statemt[26] = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = Sbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = Sbox[statemt[30] >> 4][statemt[30] & 0xf];
      statemt[30] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = Sbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = temp;

      temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = Sbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = temp;
      temp = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = Sbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = temp;
      temp = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = Sbox[statemt[27] >> 4][statemt[27] & 0xf];
      statemt[27] = temp;
      temp = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = Sbox[statemt[31] >> 4][statemt[31] & 0xf];
      statemt[31] = temp;

      statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = Sbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = Sbox[statemt[20] >> 4][statemt[20] & 0xf];
      statemt[24] = Sbox[statemt[24] >> 4][statemt[24] & 0xf];
      statemt[28] = Sbox[statemt[28] >> 4][statemt[28] & 0xf];
      break;
    }
}