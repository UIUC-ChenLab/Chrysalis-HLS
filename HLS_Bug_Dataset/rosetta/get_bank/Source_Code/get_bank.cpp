uint5_t get_bank(uint10_t addr)
{
  #pragma HLS inline
  return bank_mapping[addr];
}