int classifier48( int_II II[WINDOW_SIZE][WINDOW_SIZE], int stddev ){
  static int coord[12];
  #pragma HLS array_partition variable=coord complete dim=0

  int sum0, sum1, sum2, final_sum;
  int return_value;
  coord[0] = II[5][5];
  coord[1] = II[5][19];
  coord[2] = II[19][5];
  coord[3] = II[19][19];

  coord[4] = II[5][12];
  coord[5] = II[5][19];
  coord[6] = II[12][12];
  coord[7] = II[12][19];

  coord[8] = II[12][5];
  coord[9] = II[12][12];
  coord[10] = II[19][5];
  coord[11] = II[19][12];

  sum0 = (coord[0] - coord[1] - coord[2] + coord[3]) * -4096;
  sum1 = (coord[4] - coord[5] - coord[6] + coord[7]) * 8192;
  sum2 = (coord[8] - coord[9] - coord[10] + coord[11]) * 8192;
  final_sum = sum0 + sum1 + sum2;

  if(final_sum >= (-322 * stddev))
     return_value = 72;
  else
     return_value = -447;
  return return_value;
}