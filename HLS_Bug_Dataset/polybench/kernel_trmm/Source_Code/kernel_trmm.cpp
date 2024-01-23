static
void kernel_trmm(int ni,
		 DATA_TYPE alpha,
		 DATA_TYPE POLYBENCH_2D(A,NI,NI,ni,ni),
		 DATA_TYPE POLYBENCH_2D(B,NI,NI,ni,ni))
{
  int i, j, k;

#pragma scop
  /*  B := alpha*A'*B, A triangular */
  for (i = 1; i < _PB_NI; i++)
    for (j = 0; j < _PB_NI; j++)
      for (k = 0; k < i; k++)
        B[i][j] += alpha * A[i][k] * B[j][k];
#pragma endscop

}