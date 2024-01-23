int
AddRoundKey (int statemt[32], int type, int n)
{
  int j, nb;

  switch (type)
    {
    case 128128:
    case 192128:
    case 256128:
      nb = 4;
      break;
    case 128192:
    case 192192:
    case 256192:
      nb = 6;
      break;
    case 128256:
    case 192256:
    case 256256:
      nb = 8;
      break;
    }
  for (j = 0; j < nb; ++j)
    {
      statemt[j * 4] ^= word[0][j + nb * n];
      statemt[1 + j * 4] ^= word[1][j + nb * n];
      statemt[2 + j * 4] ^= word[2][j + nb * n];
      statemt[3 + j * 4] ^= word[3][j + nb * n];
    }
  return 0;
}