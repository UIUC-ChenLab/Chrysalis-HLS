static
void kernel_gramschmidt(int ni, int nj,
			DATA_TYPE POLYBENCH_2D(A,NI,NJ,ni,nj),
			DATA_TYPE POLYBENCH_2D(R,NJ,NJ,nj,nj),
			DATA_TYPE POLYBENCH_2D(Q,NI,NJ,ni,nj))
{
  int i, j, k;

  DATA_TYPE nrm;

#pragma scop
  for (k = 0; k < _PB_NJ; k++)
    {
      nrm = 0;
      for (i = 0; i < _PB_NI; i++)
        nrm += A[i][k] * A[i][k];
      R[k][k] = sqrt(nrm);
      for (i = 0; i < _PB_NI; i++)
        Q[i][k] = A[i][k] / R[k][k];
      for (j = k + 1; j < _PB_NJ; j++)
	{
	  R[k][j] = 0;
	  for (i = 0; i < _PB_NI; i++)
	    R[k][j] += Q[i][k] * A[i][j];
	  for (i = 0; i < _PB_NI; i++)
	    A[i][j] = A[i][j] - Q[i][k] * R[k][j];
	}
    }
#pragma endscop

}