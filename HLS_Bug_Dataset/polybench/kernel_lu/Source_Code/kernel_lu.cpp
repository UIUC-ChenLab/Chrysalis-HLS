static
void kernel_lu(int n,
	       DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int i, j, k;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      for (j = k + 1; j < _PB_N; j++)
	A[k][j] = A[k][j] / A[k][k];
      for(i = k + 1; i < _PB_N; i++)
	for (j = k + 1; j < _PB_N; j++)
	  A[i][j] = A[i][j] - A[i][k] * A[k][j];
    }
#pragma endscop

}