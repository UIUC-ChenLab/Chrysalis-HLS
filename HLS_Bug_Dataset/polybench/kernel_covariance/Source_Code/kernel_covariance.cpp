static
void kernel_covariance(int m, int n,
		       DATA_TYPE float_n,
		       DATA_TYPE POLYBENCH_2D(data,M,N,m,n),
		       DATA_TYPE POLYBENCH_2D(symmat,M,M,m,m),
		       DATA_TYPE POLYBENCH_1D(mean,M,m))
{
  int i, j, j1, j2;

#pragma scop
  /* Determine mean of column vectors of input data matrix */
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = 0.0;
      for (i = 0; i < _PB_N; i++)
        mean[j] += data[i][j];
      mean[j] /= float_n;
    }

  /* Center the column vectors. */
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      data[i][j] -= mean[j];

  /* Calculate the m * m covariance matrix. */
  for (j1 = 0; j1 < _PB_M; j1++)
    for (j2 = j1; j2 < _PB_M; j2++)
      {
        symmat[j1][j2] = 0.0;
        for (i = 0; i < _PB_N; i++)
	  symmat[j1][j2] += data[i][j1] * data[i][j2];
        symmat[j2][j1] = symmat[j1][j2];
      }
#pragma endscop

}