void linear_buffered(T input[in_size],
                     T output[out_size],
                     T weight[out_size][in_size],
                     T bias[out_size])
{
#pragma HLS INLINE off

    static_assert(in_size % BLOCK_SIZE_IN_ == 0, "in_size must be divisible by BLOCK_SIZE_IN");
    static_assert(out_size % BLOCK_SIZE_OUT_ == 0, "out_size must be divisible by BLOCK_SIZE_OUT");

    const int BLOCK_SIZE_OUT = BLOCK_SIZE_OUT_;
    const int BLOCK_SIZE_IN = BLOCK_SIZE_IN_;

#pragma HLS array_partition variable = input cyclic factor = BLOCK_SIZE_IN dim = 1
#pragma HLS array_partition variable = output cyclic factor = BLOCK_SIZE_OUT dim = 1

#pragma HLS array_partition variable = weight cyclic factor = BLOCK_SIZE_OUT dim = 1
#pragma HLS array_partition variable = weight cyclic factor = BLOCK_SIZE_IN dim = 2

#pragma HLS array_partition variable = bias cyclic factor = BLOCK_SIZE_OUT dim = 1

    // block parallel linear layer
    // use temp sum
    F_TYPE temp_sum[BLOCK_SIZE_OUT];
#pragma HLS ARRAY_PARTITION variable = temp_sum complete

    // output buffer
    F_TYPE output_buffer[out_size];
#pragma HLS array_partition variable = output_buffer cyclic factor = BLOCK_SIZE_OUT dim = 1

    // set bias on output
BIAS_BLOCK_OUT:
    for (int a = 0; a < out_size; a += BLOCK_SIZE_OUT)
    {
#pragma HLS unroll off = true
#pragma HLS PIPELINE
    BIAS_WRITE:
        for (int b = 0; b < BLOCK_SIZE_OUT; b++)
        {
#pragma HLS unroll
            int tmp_idx = a + b;
            output_buffer[tmp_idx] = bias[tmp_idx];
        }
    }

BLOCK_OUT:
    for (int i = 0; i < out_size; i += BLOCK_SIZE_OUT)
    {
    BLOCK_IN:
        for (int j = 0; j < in_size; j += BLOCK_SIZE_IN)
        {

#pragma HLS PIPELINE
        // zero temp sum
        TEMP_SUM_ZERO_LOOP:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
#pragma HLS unroll
                temp_sum[k] = 0;
            }

        // compute temp sum
        SUM_OUTER:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
            SUM_INNER:
#pragma HLS unroll
                for (int l = 0; l < BLOCK_SIZE_IN; l++)
                {
#pragma HLS unroll
                    temp_sum[k] += weight[i + k][j + l] * input[j + l];
                }
            }

        // write temp sum to output
        // aslo write bias to output on first block itteration
        WRITE_LOOP:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
#pragma HLS unroll
                output_buffer[i + k] += temp_sum[k];
            }
        }
    }

// write output buffer to output
WRITE_OUTPUT_LOOP:
    for (int i = 0; i < out_size; i++)
    {
        output[i] = output_buffer[i];
    }
}

// Content of called function
void linear(T input[in_size],
            T output[out_size],
            T weight[out_size][in_size],
            T bias[out_size])
{
#pragma HLS INLINE off

    static_assert(in_size % BLOCK_SIZE_IN_ == 0, "in_size must be divisible by BLOCK_SIZE_IN");
    static_assert(out_size % BLOCK_SIZE_OUT_ == 0, "out_size must be divisible by BLOCK_SIZE_OUT");

    const int BLOCK_SIZE_OUT = BLOCK_SIZE_OUT_;
    const int BLOCK_SIZE_IN = BLOCK_SIZE_IN_;

#pragma HLS array_partition variable = input cyclic factor = BLOCK_SIZE_IN dim = 1
#pragma HLS array_partition variable = output cyclic factor = BLOCK_SIZE_OUT dim = 1

#pragma HLS array_partition variable = weight cyclic factor = BLOCK_SIZE_OUT dim = 1
#pragma HLS array_partition variable = weight cyclic factor = BLOCK_SIZE_IN dim = 2

#pragma HLS array_partition variable = bias cyclic factor = BLOCK_SIZE_OUT dim = 1

    // block parallel linear layer
    // use temp sum
    F_TYPE temp_sum[BLOCK_SIZE_OUT];

#pragma HLS ARRAY_PARTITION variable = temp_sum complete

    // set bias on output
    // BIAS_BLOCK_OUT:
    //     for (int i = 0; i < out_size; i += BLOCK_SIZE_OUT)
    //     {

    //     #pragma HLS PIPELINE
    //         for (int j = 0; j < BLOCK_SIZE_OUT; j++)
    //         {
    //             output[i + j] = bias[i + j];
    //         }
    //     }

    // set bias on output
BIAS_BLOCK_OUT:
    for (int a = 0; a < out_size; a += BLOCK_SIZE_OUT)
    {
#pragma HLS unroll off = true
#pragma HLS PIPELINE
    BIAS_WRITE:
        for (int b = 0; b < BLOCK_SIZE_OUT; b++)
        {
#pragma HLS unroll
            int tmp_idx = a + b;
            output[tmp_idx] = bias[tmp_idx];
        }
    }

BLOCK_OUT:
    for (int i = 0; i < out_size; i += BLOCK_SIZE_OUT)
    {
    BLOCK_IN:
        for (int j = 0; j < in_size; j += BLOCK_SIZE_IN)
        {

#pragma HLS PIPELINE
        // zero temp sum
        TEMP_SUM_ZERO_LOOP:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
#pragma HLS unroll
                temp_sum[k] = 0;
            }

        // compute temp sum
        SUM_OUTER:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
#pragma HLS unroll
            SUM_INNER:
                for (int l = 0; l < BLOCK_SIZE_IN; l++)
                {
#pragma HLS unroll
                    temp_sum[k] += weight[i + k][j + l] * input[j + l];
                }
            }

        // write temp sum to output
        // aslo write bias to output on first block itteration
        WRITE_LOOP:
            for (int k = 0; k < BLOCK_SIZE_OUT; k++)
            {
#pragma HLS unroll
                output[i + k] += temp_sum[k];
            }
        }
    }
}