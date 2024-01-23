word
gsm_mult_r (word a, word b)
{
  longword prod;
  if (b == MIN_WORD && a == MIN_WORD)
    return MAX_WORD;
  else
    {
      prod = (longword) a *(longword) b + 16384;
      prod >>= 15;
      return prod & 0xFFFF;
    }
}