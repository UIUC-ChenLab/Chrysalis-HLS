void
sha_stream ()
{
  int i, j;
  const BYTE *p;

  sha_init ();
  for (j = 0; j < VSIZE; j++)
    {
      i = in_i[j];
      p = &indata[j][0];
      sha_update (p, i);
    }
  sha_final ();
}

// Content of called function
void
sha_update (const BYTE * buffer, int count)
{
  if ((sha_info_count_lo + ((INT32) count << 3)) < sha_info_count_lo)
    {
      ++sha_info_count_hi;
    }
  sha_info_count_lo += (INT32) count << 3;
  sha_info_count_hi += (INT32) count >> 29;
  while (count >= SHA_BLOCKSIZE)
    {
      local_memcpy (sha_info_data, buffer, SHA_BLOCKSIZE);
      sha_transform ();
      buffer += SHA_BLOCKSIZE;
      count -= SHA_BLOCKSIZE;
    }
  local_memcpy (sha_info_data, buffer, count);
}

// Content of called function
void
local_memcpy (INT32 * s1, const BYTE * s2, int n)
{
  INT32 *p1;
  BYTE *p2;
  INT32 tmp;
  int m;
  m = n / 4;
  p1 = (INT32 *) s1;
  p2 = (BYTE *) s2;

  while (m-- > 0)
    {
      tmp = 0;
      tmp |= 0xFF & *p2++;
      tmp |= (0xFF & *p2++) << 8;
      tmp |= (0xFF & *p2++) << 16;
      tmp |= (0xFF & *p2++) << 24;
      *p1 = tmp;
      p1++;
    }
}

// Content of called function
sha_transform ()
{
  int i;
  INT32 temp, A, B, C, D, E, W[80];

  for (i = 0; i < 16; ++i)
    {
      W[i] = sha_info_data[i];
    }
  for (i = 16; i < 80; ++i)
    {
      W[i] = W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16];
    }
  A = sha_info_digest[0];
  B = sha_info_digest[1];
  C = sha_info_digest[2];
  D = sha_info_digest[3];
  E = sha_info_digest[4];

  for (i = 0; i < 20; ++i)
    {
      FUNC (1, i);
    }
  for (i = 20; i < 40; ++i)
    {
      FUNC (2, i);
    }
  for (i = 40; i < 60; ++i)
    {
      FUNC (3, i);
    }
  for (i = 60; i < 80; ++i)
    {
      FUNC (4, i);
    }

  sha_info_digest[0] += A;
  sha_info_digest[1] += B;
  sha_info_digest[2] += C;
  sha_info_digest[3] += D;
  sha_info_digest[4] += E;
}

// Content of called function
void
sha_final ()
{
  int count;
  INT32 lo_bit_count;
  INT32 hi_bit_count;

  lo_bit_count = sha_info_count_lo;
  hi_bit_count = sha_info_count_hi;
  count = (int) ((lo_bit_count >> 3) & 0x3f);
  sha_info_data[count++] = 0x80;
  if (count > 56)
    {
      local_memset (sha_info_data, 0, 64 - count, count);
      sha_transform ();
      local_memset (sha_info_data, 0, 56, 0);
    }
  else
    {
      local_memset (sha_info_data, 0, 56 - count, count);
    }
  sha_info_data[14] = hi_bit_count;
  sha_info_data[15] = lo_bit_count;
  sha_transform ();
}

// Content of called function
void
local_memset (INT32 * s, int c, int n, int e)
{
  INT32 uc;
  INT32 *p;
  int m;

  m = n / 4;
  uc = c;
  p = (INT32 *) s;
  while (e-- > 0)
    {
      p++;
    }
  while (m-- > 0)
    {
      *p++ = uc;
    }
}

// Content of called function
void
sha_init ()
{
  sha_info_digest[0] = 0x67452301L;
  sha_info_digest[1] = 0xefcdab89L;
  sha_info_digest[2] = 0x98badcfeL;
  sha_info_digest[3] = 0x10325476L;
  sha_info_digest[4] = 0xc3d2e1f0L;
  sha_info_count_lo = 0L;
  sha_info_count_hi = 0L;
}