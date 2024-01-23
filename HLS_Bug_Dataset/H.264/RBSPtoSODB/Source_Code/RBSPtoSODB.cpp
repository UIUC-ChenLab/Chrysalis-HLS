void RBSPtoSODB(NALU_t *nalu, int len)
{
  unsigned char i;

  for(i=0;i<8;i++)
  {
    if( nalu->buf[len] & 0x01<<i)
    {
      break;
    }
  }

  nalu->bit_length=len*8+7-i;
}