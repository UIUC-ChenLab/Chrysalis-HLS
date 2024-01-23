static
void kernel_dynprog(int tsteps, int length,
		    DATA_TYPE POLYBENCH_2D(c,LENGTH,LENGTH,length,length),
		    DATA_TYPE POLYBENCH_2D(W,LENGTH,LENGTH,length,length),
		    DATA_TYPE POLYBENCH_3D(sum_c,LENGTH,LENGTH,LENGTH,length,length,length),
		    DATA_TYPE *out)
{
  int iter, i, j, k;

  DATA_TYPE out_l = 0;

#pragma scop
  for (iter = 0; iter < _PB_TSTEPS; iter++)
    {
      for (i = 0; i <= _PB_LENGTH - 1; i++)
	for (j = 0; j <= _PB_LENGTH - 1; j++)
	  c[i][j] = 0;

      for (i = 0; i <= _PB_LENGTH - 2; i++)
	{
	  for (j = i + 1; j <= _PB_LENGTH - 1; j++)
	    {
	      sum_c[i][j][i] = 0;
	      for (k = i + 1; k <= j-1; k++)
		sum_c[i][j][k] = sum_c[i][j][k - 1] + c[i][k] + c[k][j];
	      c[i][j] = sum_c[i][j][j-1] + W[i][j];
	    }
	}
      out_l += c[0][_PB_LENGTH - 1];
    }
#pragma endscop

  *out = out_l;
}