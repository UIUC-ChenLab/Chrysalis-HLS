void gine_conv_agg(
    int num_in_neighbors,
    int neighbors[MAX_NODES],
    int neighbor_edge_indexs[MAX_NODES],
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T edge_feature_table[MAX_EDGES][EDGE_FEATURE_SIZE],
    T agg_embedding[EMB_SIZE_IN],
    T edge_proj_weight[EMB_SIZE_IN][EDGE_FEATURE_SIZE],
    T edge_proj_bias[EMB_SIZE_IN])
{
#pragma HLS INLINE off

    sum_incremental_data<T> neighbor_emb_aggregation_sum[EMB_SIZE_IN];
    for (int neighbor = 0; neighbor < num_in_neighbors; neighbor++)
    {
#pragma HLS loop_tripcount min = 1 max = DEGREE_GUESS

        T neighbor_emb[EMB_SIZE_IN];
        int neighbor_id = neighbors[neighbor];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_emb[i] = node_embedding_table_in[neighbor_id][i];
        }

        T edge_feature[EDGE_FEATURE_SIZE];
        int edge_index = neighbor_edge_indexs[neighbor];
        for (int i = 0; i < EDGE_FEATURE_SIZE; i++)
        {
            edge_feature[i] = edge_feature_table[edge_index][i];
        }

        T edge_proj[EMB_SIZE_IN];
        linear<EDGE_FEATURE_SIZE, EMB_SIZE_IN, P_IN, P_IN, T>(edge_feature, edge_proj, edge_proj_weight, edge_proj_bias);

        T message[EMB_SIZE_IN];
        merge_sum_1d<EMB_SIZE_IN>(neighbor_emb, edge_proj, message);

        T message_act[EMB_SIZE_IN];
        apply_activation_1d<EMB_SIZE_IN, T, activation_relu>(message, message_act);

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            sum_incremental_update(neighbor_emb_aggregation_sum[i], message_act[i]);
        }
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        sum_incremental_finalize(neighbor_emb_aggregation_sum[i]);
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        agg_embedding[i] = neighbor_emb_aggregation_sum[i].sum;
    }
}

// Content of called function
T activation_relu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    if (x > 0)
    {
        return x;
    }
    else
    {
        return 0;
    }
}

// Content of called function
void sum_incremental_finalize(sum_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}

// Content of called function
void sum_incremental_update(sum_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
}

// Content of called function
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

// Content of called function
void apply_activation_1d(T_array_1d<T, N> x_in, T_array_1d<T, N> x_out)
{
#pragma HLS INLINE off
    for (int i = 0; i < N; i++)
    {
        x_out[i] = T_func(x_in[i]);
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