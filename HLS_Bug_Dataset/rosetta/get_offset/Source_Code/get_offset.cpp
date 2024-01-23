uint5_t get_offset(uint10_t addr)
{
  #pragma HLS inline
  return offset_mapping[addr];
}