void merge_sum_3d(T_array_3d<T, M, N, O> x_in_1, T_array_3d<T, M, N, O> x_in_2, T_array_3d<T, M, N, O> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            for (int k = 0; k < O; k++)
            {
                T x_in_1_buf = x_in_1[i][j][k];
                T x_in_2_buf = x_in_2[i][j][k];
                x_out[i][j][k] = x_in_1_buf + x_in_2_buf;
            }
        }
    }
}