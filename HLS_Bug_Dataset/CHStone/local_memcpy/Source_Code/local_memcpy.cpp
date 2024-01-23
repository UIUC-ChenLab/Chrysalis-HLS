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