static
void kernel_jacobi_1d_imper(int tsteps,
			    int n,
			    DATA_TYPE POLYBENCH_1D(A,N,n),
			    DATA_TYPE POLYBENCH_1D(B,N,n))
{
  int t, i, j;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++)
    {
      for (i = 1; i < _PB_N - 1; i++)
	B[i] = 0.33333 * (A[i-1] + A[i] + A[i + 1]);
      for (j = 1; j < _PB_N - 1; j++)
	A[j] = B[j];
    }
#pragma endscop

}