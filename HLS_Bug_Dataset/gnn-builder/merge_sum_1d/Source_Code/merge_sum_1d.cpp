void merge_sum_1d(T_array_1d<T, N> x_in_1, T_array_1d<T, N> x_in_2, T_array_1d<T, N> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < N; i++)
    {
        T x_in_1_buf = x_in_1[i];
        T x_in_2_buf = x_in_2[i];
        x_out[i] = x_in_1_buf + x_in_2_buf;
    }
}