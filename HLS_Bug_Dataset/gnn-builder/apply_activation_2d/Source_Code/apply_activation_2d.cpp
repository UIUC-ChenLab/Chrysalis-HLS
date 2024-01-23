void apply_activation_2d(T_array_2d<T, M, N> x_in, T_array_2d<T, M, N> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {

            x_out[i][j] = T_func(x_in[i][j]);
        }
    }
}