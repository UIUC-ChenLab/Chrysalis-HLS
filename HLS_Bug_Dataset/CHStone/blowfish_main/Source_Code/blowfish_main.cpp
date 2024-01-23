
#define N 40

#define KEYSIZE 5200

int
blowfish_main ()
{
  unsigned char ukey[8];
  unsigned char indata[N];
  unsigned char outdata[N];
  unsigned char ivec[8];
  int num;
  int i, j, k, l;
  int encordec;
  int check;

  num = 0;
  k = 0;
  l = 0;
  encordec = 1;
  check = 0;
  for (i = 0; i < 8; i++)
    {
      ukey[i] = 0;
      ivec[i] = 0;
    }
  BF_set_key (8, ukey);
  i = 0;
  while (k < KEYSIZE)
    {
      while (k < KEYSIZE && i < N)
	indata[i++] = in_key[k++];

      BF_cfb64_encrypt (indata, outdata, i, ivec, &num, encordec);

      for (j = 0; j < i; j++)
	check += (outdata[j] != out_key[l++]);

      i = 0;
    }
  return check;
}