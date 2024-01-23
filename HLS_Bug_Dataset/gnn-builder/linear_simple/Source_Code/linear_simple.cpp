void linear_simple(T_array_1d<T, in_size> input,
                   T_array_1d<T, out_size> output,
                   T_linear_weight_array<T, in_size, out_size> weight,
                   T_linear_bias_array<T, out_size> bias)
{
#pragma HLS INLINE off

    for (int i = 0; i < out_size; i++)
    {
        output[i] = bias[i];
        for (int j = 0; j < in_size; j++)
        {
            output[i] += weight[i][j] * input[j];
        }
    }
}