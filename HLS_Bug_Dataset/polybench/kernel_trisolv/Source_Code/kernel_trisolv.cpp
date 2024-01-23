static
void kernel_trisolv(int n,
		    DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(c,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      x[i] = c[i];
      for (j = 0; j <= i - 1; j++)
        x[i] = x[i] - A[i][j] * x[j];
      x[i] = x[i] / A[i][i];
    }
#pragma endscop

}