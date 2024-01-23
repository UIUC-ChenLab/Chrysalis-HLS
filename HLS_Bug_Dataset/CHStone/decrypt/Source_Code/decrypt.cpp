int
decrypt (int statemt[32], int key[32], int type)
{
  int i;
/*
+--------------------------------------------------------------------------+
| * Test Vector (added for CHStone)                                        |
|     out_enc_statemt : expected output data for "decrypt"                 |
+--------------------------------------------------------------------------+
*/
  const int out_dec_statemt[16] =
    { 0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2,
    0xe0, 0x37, 0x7, 0x34
  };
  KeySchedule (type, key);

  switch (type)
    {
    case 128128:
      round_val = 10;
      nb = 4;
      break;
    case 128192:
    case 192192:
      round_val = 12;
      nb = 6;
      break;
    case 192128:
      round_val = 12;
      nb = 4;
      break;
    case 128256:
    case 192256:
      round_val = 14;
      nb = 8;
      break;
    case 256128:
      round_val = 14;
      nb = 4;
      break;
    case 256192:
      round_val = 14;
      nb = 6;
      break;
    case 256256:
      round_val = 14;
      nb = 8;
      break;
    }

  AddRoundKey (statemt, type, round_val);

  InversShiftRow_ByteSub (statemt, nb);

  for (i = round_val - 1; i >= 1; --i)
    {
      AddRoundKey_InversMixColumn (statemt, nb, i);
      InversShiftRow_ByteSub (statemt, nb);
    }

  AddRoundKey (statemt, type, 0);

  printf ("\ndecrypto message\t");
  for (i = 0; i < ((type % 1000) / 8); ++i)
    {
      if (statemt[i] < 16)
	printf ("0");
      printf ("%x", statemt[i]);
    }

  for (i = 0; i < 16; i++)
    main_result += (statemt[i] != out_dec_statemt[i]);

  return 0;
}