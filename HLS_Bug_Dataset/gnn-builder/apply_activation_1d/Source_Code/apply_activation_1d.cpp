void apply_activation_1d(T_array_1d<T, N> x_in, T_array_1d<T, N> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < N; i++)
    {
        x_out[i] = T_func(x_in[i]);
    }
}