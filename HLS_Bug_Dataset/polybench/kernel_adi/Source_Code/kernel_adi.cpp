static
void kernel_adi(int tsteps,
		int n,
		DATA_TYPE POLYBENCH_2D(X,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(B,N,N,n,n))
{
  int t, i1, i2;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++)
    {
      for (i1 = 0; i1 < _PB_N; i1++)
	for (i2 = 1; i2 < _PB_N; i2++)
	  {
	    X[i1][i2] = X[i1][i2] - X[i1][i2-1] * A[i1][i2] / B[i1][i2-1];
	    B[i1][i2] = B[i1][i2] - A[i1][i2] * A[i1][i2] / B[i1][i2-1];
	  }

      for (i1 = 0; i1 < _PB_N; i1++)
	X[i1][_PB_N-1] = X[i1][_PB_N-1] / B[i1][_PB_N-1];

      for (i1 = 0; i1 < _PB_N; i1++)
	for (i2 = 0; i2 < _PB_N-2; i2++)
	  X[i1][_PB_N-i2-2] = (X[i1][_PB_N-2-i2] - X[i1][_PB_N-2-i2-1] * A[i1][_PB_N-i2-3]) / B[i1][_PB_N-3-i2];

      for (i1 = 1; i1 < _PB_N; i1++)
	for (i2 = 0; i2 < _PB_N; i2++) {
	  X[i1][i2] = X[i1][i2] - X[i1-1][i2] * A[i1][i2] / B[i1-1][i2];
	  B[i1][i2] = B[i1][i2] - A[i1][i2] * A[i1][i2] / B[i1-1][i2];
	}

      for (i2 = 0; i2 < _PB_N; i2++)
	X[_PB_N-1][i2] = X[_PB_N-1][i2] / B[_PB_N-1][i2];

      for (i1 = 0; i1 < _PB_N-2; i1++)
	for (i2 = 0; i2 < _PB_N; i2++)
	  X[_PB_N-2-i1][i2] = (X[_PB_N-2-i1][i2] - X[_PB_N-i1-3][i2] * A[_PB_N-3-i1][i2]) / B[_PB_N-2-i1][i2];
    }
#pragma endscop

}