void split_3d(T_array_3d<T, M, N, O> x_in, T_array_3d<T, M, N, O> x_out_1, T_array_3d<T, M, N, O> x_out_2)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            for (int k = 0; k < O; k++)
            {
                T x_in_buf = x_in[i][j][k];
                x_out_1[i][j][k] = x_in_buf;
                x_out_2[i][j][k] = x_in_buf;
            }
        }
    }
}