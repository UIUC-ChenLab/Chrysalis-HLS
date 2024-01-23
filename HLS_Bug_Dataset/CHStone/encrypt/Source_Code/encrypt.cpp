int
encrypt (int statemt[32], int key[32], int type)
{
  int i;
/*
+--------------------------------------------------------------------------+
| * Test Vector (added for CHStone)                                        |
|     out_enc_statemt : expected output data for "encrypt"                 |
+--------------------------------------------------------------------------+
*/
  const int out_enc_statemt[16] =
    { 0x39, 0x25, 0x84, 0x1d, 0x2, 0xdc, 0x9, 0xfb, 0xdc, 0x11, 0x85, 0x97,
    0x19, 0x6a, 0xb, 0x32
  };

  KeySchedule (type, key);
  switch (type)
    {
    case 128128:
      round_val = 0;
      nb = 4;
      break;
    case 192128:
      round_val = 2;
      nb = 4;
      break;
    case 256128:
      round_val = 4;
      nb = 4;
      break;
    case 128192:
    case 192192:
      round_val = 2;
      nb = 6;
      break;
    case 256192:
      round_val = 4;
      nb = 6;
      break;
    case 128256:
    case 192256:
    case 256256:
      round_val = 4;
      nb = 8;
      break;
    }
  AddRoundKey (statemt, type, 0);
  for (i = 1; i <= round_val + 9; ++i)
    {
      ByteSub_ShiftRow (statemt, nb);
      MixColumn_AddRoundKey (statemt, nb, i);
    }
  ByteSub_ShiftRow (statemt, nb);
  AddRoundKey (statemt, type, i);

  printf ("encrypted message \t");
  for (i = 0; i < nb * 4; ++i)
    {
      if (statemt[i] < 16)
	printf ("0");
      printf ("%x", statemt[i]);
    }

  for (i = 0; i < 16; i++)
    main_result += (statemt[i] != out_enc_statemt[i]);

  return 0;
}