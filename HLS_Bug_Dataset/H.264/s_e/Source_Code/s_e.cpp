int s_e(NALU_t* nalu)
{
  int ret;
  ret=u_e(nalu);
  if(ret % 2)
    return (ret+1)/2;
  else
    return -(ret+1)/2;
}