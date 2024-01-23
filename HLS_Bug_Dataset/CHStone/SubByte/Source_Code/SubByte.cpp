int
SubByte (int in)
{
  return Sbox[(in / 16)][(in % 16)];
}