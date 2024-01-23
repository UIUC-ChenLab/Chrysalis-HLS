static
void kernel_atax(int nx, int ny,
		 DATA_TYPE POLYBENCH_2D(A,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_1D(x,NY,ny),
		 DATA_TYPE POLYBENCH_1D(y,NY,ny),
		 DATA_TYPE POLYBENCH_1D(tmp,NX,nx))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_NY; i++)
    y[i] = 0;
  for (i = 0; i < _PB_NX; i++)
    {
      tmp[i] = 0;
      for (j = 0; j < _PB_NY; j++)
	tmp[i] = tmp[i] + A[i][j] * x[j];
      for (j = 0; j < _PB_NY; j++)
	y[j] = y[j] + A[i][j] * tmp[i];
    }
#pragma endscop

}