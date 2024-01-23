int
KeySchedule (int type, int key[32])
{
  int nk, nb, round_val;
  int i, j, temp[4];

  switch (type)
    {
    case 128128:
      nk = 4;
      nb = 4;
      round_val = 10;
      break;
    case 128192:
      nk = 4;
      nb = 6;
      round_val = 12;
      break;
    case 128256:
      nk = 4;
      nb = 8;
      round_val = 14;
      break;
    case 192128:
      nk = 6;
      nb = 4;
      round_val = 12;
      break;
    case 192192:
      nk = 6;
      nb = 6;
      round_val = 12;
      break;
    case 192256:
      nk = 6;
      nb = 8;
      round_val = 14;
      break;
    case 256128:
      nk = 8;
      nb = 4;
      round_val = 14;
      break;
    case 256192:
      nk = 8;
      nb = 6;
      round_val = 14;
      break;
    case 256256:
      nk = 8;
      nb = 8;
      round_val = 14;
      break;
    default:
      return -1;
    }
  for (j = 0; j < nk; ++j)
    for (i = 0; i < 4; ++i)
/* 0 word */
      word[i][j] = key[i + j * 4];

/* expanded key is generated */
  for (j = nk; j < nb * (round_val + 1); ++j)
    {

/* RotByte */
      if ((j % nk) == 0)
	{
	  temp[0] = SubByte (word[1][j - 1]) ^ Rcon0[(j / nk) - 1];
	  temp[1] = SubByte (word[2][j - 1]);
	  temp[2] = SubByte (word[3][j - 1]);
	  temp[3] = SubByte (word[0][j - 1]);
	}
      if ((j % nk) != 0)
	{
	  temp[0] = word[0][j - 1];
	  temp[1] = word[1][j - 1];
	  temp[2] = word[2][j - 1];
	  temp[3] = word[3][j - 1];
	}
      if (nk > 6 && j % nk == 4)
	for (i = 0; i < 4; ++i)
	  temp[i] = SubByte (temp[i]);
      for (i = 0; i < 4; ++i)
	word[i][j] = word[i][j - nk] ^ temp[i];
    }
  return 0;
}

// Content of called function
int
SubByte (int in)
{
  return Sbox[(in / 16)][(in % 16)];
}