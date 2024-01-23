void merge_sum_2d(T_array_2d<T, M, N> x_in_1, T_array_2d<T, M, N> x_in_2, T_array_2d<T, M, N> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            T x_in_1_buf = x_in_1[i][j];
            T x_in_2_buf = x_in_2[i][j];
            x_out[i][j] = x_in_1_buf + x_in_2_buf;
        }
    }
}