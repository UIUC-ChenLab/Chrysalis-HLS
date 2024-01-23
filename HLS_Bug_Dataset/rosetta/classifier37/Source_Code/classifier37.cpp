int classifier37( int_II II[WINDOW_SIZE][WINDOW_SIZE], int stddev ){
  static int coord[12];
  #pragma HLS array_partition variable=coord complete dim=0

  int sum0, sum1, sum2, final_sum;
  int return_value;
  coord[0] = II[1][8];
  coord[1] = II[1][12];
  coord[2] = II[11][8];
  coord[3] = II[11][12];

  coord[4] = II[1][10];
  coord[5] = II[1][12];
  coord[6] = II[11][10];
  coord[7] = II[11][12];

  coord[8] = 0;
  coord[9] = 0;
  coord[10] = 0;
  coord[11] = 0;

  sum0 = (coord[0] - coord[1] - coord[2] + coord[3]) * -4096;
  sum1 = (coord[4] - coord[5] - coord[6] + coord[7]) * 8192;
  sum2 = (coord[8] - coord[9] - coord[10] + coord[11]) * 0;
  final_sum = sum0 + sum1 + sum2;

  if(final_sum >= (-7 * stddev))
     return_value = 59;
  else
     return_value = -259;
  return return_value;
}