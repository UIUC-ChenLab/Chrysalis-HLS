static
void kernel_durbin(int n,
		   DATA_TYPE POLYBENCH_2D(y,N,N,n,n),
		   DATA_TYPE POLYBENCH_2D(sum,N,N,n,n),
		   DATA_TYPE POLYBENCH_1D(alpha,N,n),
		   DATA_TYPE POLYBENCH_1D(beta,N,n),
		   DATA_TYPE POLYBENCH_1D(r,N,n),
		   DATA_TYPE POLYBENCH_1D(out,N,n))
{
  int i, k;

#pragma scop
  y[0][0] = r[0];
  beta[0] = 1;
  alpha[0] = r[0];
  for (k = 1; k < _PB_N; k++)
    {
      beta[k] = beta[k-1] - alpha[k-1] * alpha[k-1] * beta[k-1];
      sum[0][k] = r[k];
      for (i = 0; i <= k - 1; i++)
	sum[i+1][k] = sum[i][k] + r[k-i-1] * y[i][k-1];
      alpha[k] = -sum[k][k] * beta[k];
      for (i = 0; i <= k-1; i++)
	y[i][k] = y[i][k-1] + alpha[k] * y[k-i-1][k-1];
      y[k][k] = alpha[k];
    }
  for (i = 0; i < _PB_N; i++)
    out[i] = y[i][_PB_N-1];
#pragma endscop

}