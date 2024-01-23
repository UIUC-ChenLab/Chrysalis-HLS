void pna_conv_agg(
    int num_in_neighbors,
    int neighbors[MAX_NODES],
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T current_node_embedding_in[EMB_SIZE_IN],
    T neighbor_emb_agg_max[EMB_SIZE_IN],
    T neighbor_emb_agg_min[EMB_SIZE_IN],
    T neighbor_emb_agg_mean[EMB_SIZE_IN],
    T neighbor_emb_agg_variance[EMB_SIZE_IN],
    T transfrom_lin_weight[TRANSFORM_OUT][TRANSFORM_IN],
    T transfrom_lin_bias[TRANSFORM_OUT])
{
#pragma HLS INLINE off

    max_incremental_data<T> neighbor_emb_aggregation_max[EMB_SIZE_IN];
    min_incremental_data<T> neighbor_emb_aggregation_min[EMB_SIZE_IN];
    mean_incremental_data<T> neighbor_emb_aggregation_mean[EMB_SIZE_IN];
    variance_incremental_data<T> neighbor_emb_aggregation_variance[EMB_SIZE_IN];

    for (int neighbor = 0; neighbor < num_in_neighbors; neighbor++)
    {
#pragma HLS loop_tripcount min = 1 max = DEGREE_GUESS

        T neighbor_emb[EMB_SIZE_IN];

        int neighbor_id = neighbors[neighbor];

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_emb[i] = node_embedding_table_in[neighbor_id][i];
        }

        T neighbor_and_self_concat_emb[EMB_SIZE_IN * 2];

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_and_self_concat_emb[i] = current_node_embedding_in[i];
            neighbor_and_self_concat_emb[i + EMB_SIZE_IN] = neighbor_emb[i];
        }

        // tansform emb
        T transformed_emb[EMB_SIZE_IN] = {};
        linear<EMB_SIZE_IN * 2, EMB_SIZE_IN, P_IN, P_IN, T>(neighbor_and_self_concat_emb, transformed_emb, transfrom_lin_weight, transfrom_lin_bias);

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            max_incremental_update(neighbor_emb_aggregation_max[i], transformed_emb[i]);
            min_incremental_update(neighbor_emb_aggregation_min[i], transformed_emb[i]);
            mean_incremental_update(neighbor_emb_aggregation_mean[i], transformed_emb[i]);
            variance_incremental_update(neighbor_emb_aggregation_variance[i], transformed_emb[i]);
        }
    }

    // finalize
    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        max_incremental_finalize(neighbor_emb_aggregation_max[i]);
        min_incremental_finalize(neighbor_emb_aggregation_min[i]);
        mean_incremental_finalize(neighbor_emb_aggregation_mean[i]);
        variance_incremental_finalize(neighbor_emb_aggregation_variance[i]);
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        neighbor_emb_agg_max[i] = neighbor_emb_aggregation_max[i].max;
        neighbor_emb_agg_min[i] = neighbor_emb_aggregation_min[i].min;
        neighbor_emb_agg_mean[i] = neighbor_emb_aggregation_mean[i].mean;
        neighbor_emb_agg_variance[i] = neighbor_emb_aggregation_variance[i].std;
    }
}

// Content of called function
void variance_incremental_finalize(variance_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.var = data.m2 / T(data.count);
    data.std = m_sqrt(data.var + T(1e-5));
    data.finalized = true;
}

// Content of called function
void max_incremental_update(max_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    if (data.one_sample_processed == false)
    {
        data.max = x;
        data.one_sample_processed = true;
    }
    else
    {
        if (x > data.max)
        {
            data.max = x;
        }
    }
}

// Content of called function
void mean_incremental_update(mean_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
    data.count++;
    data.mean = data.sum / T(data.count);
}

// Content of called function
void variance_incremental_update(variance_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.count++;
    T delta = x - data.mean;
    data.mean += delta / T(data.count);
    data.m2 += delta * (x - data.mean);
}

// Content of called function
void max_incremental_finalize(max_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}

// Content of called function
void min_incremental_update(min_incremental_data<T> &data, T x)
{
    if (data.one_sample_processed == false)
    {
        data.min = x;
        data.one_sample_processed = true;
    }
    else
    {
        if (x < data.min)
        {
            data.min = x;
        }
    }
}

// Content of called function
void mean_incremental_finalize(mean_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
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

// Content of called function
void min_incremental_finalize(min_incremental_data<T> &data)
{
    data.finalized = true;
}