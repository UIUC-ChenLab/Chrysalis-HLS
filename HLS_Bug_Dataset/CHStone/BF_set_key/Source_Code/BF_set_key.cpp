void
BF_set_key (int len, unsigned char *data)
{
  int i;
  BF_LONG *p, ri, in[2];
  unsigned char *d, *end;

  local_memcpy (key_P, bf_init_P, BF_ROUNDS + 2);
  local_memcpy (key_S, bf_init_S, 4 * 256);
  p = key_P;

  if (len > ((BF_ROUNDS + 2) * 4))
    len = (BF_ROUNDS + 2) * 4;

  d = data;
  end = &(data[len]);
  for (i = 0; i < (BF_ROUNDS + 2); i++)
    {
      ri = *(d++);
      if (d >= end)
	d = data;

      ri <<= 8;
      ri |= *(d++);
      if (d >= end)
	d = data;

      ri <<= 8;
      ri |= *(d++);
      if (d >= end)
	d = data;

      ri <<= 8;
      ri |= *(d++);
      if (d >= end)
	d = data;

      p[i] ^= ri;
    }

  in[0] = 0L;
  in[1] = 0L;
  for (i = 0; i < (BF_ROUNDS + 2); i += 2)
    {
      BF_encrypt (in, BF_ENCRYPT);
      p[i] = in[0];
      p[i + 1] = in[1];
    }

  p = key_S;
  for (i = 0; i < 4 * 256; i += 2)
    {
      BF_encrypt (in, BF_ENCRYPT);
      p[i] = in[0];
      p[i + 1] = in[1];
    }

}

// Content of called function
void
local_memcpy (BF_LONG * s1, const BF_LONG * s2, int n)
{
  BF_LONG *p1;
  const BF_LONG *p2;

  p1 = s1;
  p2 = s2;

  while (n-- > 0)
    {
      *p1 = *p2;
      p1++;
      p2++;
    }
}