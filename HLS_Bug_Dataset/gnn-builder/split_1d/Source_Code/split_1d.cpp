void split_1d(T_array_1d<T, N> x_in, T_array_1d<T, N> x_out_1, T_array_1d<T, N> x_out_2)
{
#pragma HLS INLINE off
    for (int i = 0; i < N; i++)
    {
        T x_in_buf = x_in[i];
        x_out_1[i] = x_in_buf;
        x_out_2[i] = x_in_buf;
    }
}