static
void kernel_reg_detect(int niter, int maxgrid, int length,
		       DATA_TYPE POLYBENCH_2D(sum_tang,MAXGRID,MAXGRID,maxgrid,maxgrid),
		       DATA_TYPE POLYBENCH_2D(mean,MAXGRID,MAXGRID,maxgrid,maxgrid),
		       DATA_TYPE POLYBENCH_2D(path,MAXGRID,MAXGRID,maxgrid,maxgrid),
		       DATA_TYPE POLYBENCH_3D(diff,MAXGRID,MAXGRID,LENGTH,maxgrid,maxgrid,length),
		       DATA_TYPE POLYBENCH_3D(sum_diff,MAXGRID,MAXGRID,LENGTH,maxgrid,maxgrid,length))
{
  int t, i, j, cnt;

#pragma scop
  for (t = 0; t < _PB_NITER; t++)
    {
      for (j = 0; j <= _PB_MAXGRID - 1; j++)
	for (i = j; i <= _PB_MAXGRID - 1; i++)
	  for (cnt = 0; cnt <= _PB_LENGTH - 1; cnt++)
	    diff[j][i][cnt] = sum_tang[j][i];

      for (j = 0; j <= _PB_MAXGRID - 1; j++)
        {
	  for (i = j; i <= _PB_MAXGRID - 1; i++)
            {
	      sum_diff[j][i][0] = diff[j][i][0];
	      for (cnt = 1; cnt <= _PB_LENGTH - 1; cnt++)
		sum_diff[j][i][cnt] = sum_diff[j][i][cnt - 1] + diff[j][i][cnt];
	      mean[j][i] = sum_diff[j][i][_PB_LENGTH - 1];
            }
        }

      for (i = 0; i <= _PB_MAXGRID - 1; i++)
	path[0][i] = mean[0][i];

      for (j = 1; j <= _PB_MAXGRID - 1; j++)
	for (i = j; i <= _PB_MAXGRID - 1; i++)
	  path[j][i] = path[j - 1][i - 1] + mean[j][i];
    }
#pragma endscop

}