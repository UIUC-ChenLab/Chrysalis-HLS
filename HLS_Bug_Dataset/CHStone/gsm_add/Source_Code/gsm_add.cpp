word
gsm_add (word a, word b)
{
  longword sum;
  sum = (longword) a + (longword) b;
  return saturate (sum);
}