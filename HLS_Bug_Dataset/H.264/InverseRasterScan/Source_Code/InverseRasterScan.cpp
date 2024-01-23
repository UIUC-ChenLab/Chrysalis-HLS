int InverseRasterScan(int a, int b, int c, int d, int e)
{
  if(e==0)
  {
    return (a%(d/b))*b;
  }
  else
  {
    return (a/(d/b))*c;
  }
  return 0;
}