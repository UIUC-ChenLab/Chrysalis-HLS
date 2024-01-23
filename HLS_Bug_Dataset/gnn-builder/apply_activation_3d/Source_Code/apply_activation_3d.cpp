void apply_activation_3d(T_array_3d<T, M, N, O> x_in, T_array_3d<T, M, N, O> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            for (int k = 0; k < O; k++)
            {
                x_out[i][j][k] = T_func(x_in[i][j][k]);
            }
        }
    }
}