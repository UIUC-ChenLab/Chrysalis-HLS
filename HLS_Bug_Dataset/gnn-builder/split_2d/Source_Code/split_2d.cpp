void split_2d(T_array_2d<T, M, N> x_in, T_array_2d<T, M, N> x_out_1, T_array_2d<T, M, N> x_out_2)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            T x_in_buf = x_in[i][j];
            x_out_1[i][j] = x_in_buf;
            x_out_2[i][j] = x_in_buf;
        }
    }
}