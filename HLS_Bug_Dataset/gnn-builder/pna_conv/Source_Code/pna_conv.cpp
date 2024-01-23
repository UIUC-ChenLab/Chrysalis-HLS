void pna_conv(
    int num_nodes,
    int num_edges,
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T node_embedding_table_out[MAX_NODES][EMB_SIZE_OUT],
    int edge_list[MAX_EDGES][2],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES],
    int in_degree_table[MAX_NODES],
    int out_degree_table[MAX_NODES],
    T transfrom_lin_weight[TRANSFORM_OUT][TRANSFORM_IN],
    T transfrom_lin_bias[TRANSFORM_OUT],
    T apply_lin_weight[APPLY_OUT][APPLY_IN],
    T apply_lin_bias[APPLY_OUT],
    T final_lin_weight[EMB_SIZE_OUT][EMB_SIZE_OUT],
    T final_lin_bias[EMB_SIZE_OUT],
    T pna_avg_degree_log)
{
#pragma HLS INLINE off

    for (int node = 0; node < num_nodes; node++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS

        // #pragma HLS DATAFLOW
        // #pragma HLS stable variable = edge_list
        // #pragma HLS stable variable = neightbor_table_offsets
        // #pragma HLS stable variable = neighbor_table
        // #pragma HLS stable variable = in_degree_table
        // #pragma HLS stable variable = out_degree_table

        T current_node_embedding_in[EMB_SIZE_IN];
        T current_node_embedding_in_for_agg[EMB_SIZE_IN];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            T x_in = node_embedding_table_in[node][i];
            current_node_embedding_in[i] = x_in;
            current_node_embedding_in_for_agg[i] = x_in;
        }

        int num_in_neighbors;
        int num_in_neighbors_0;
        int num_in_neighbors_1;
        int num_in_neighbors_2;

        num_in_neighbors = in_degree_table[node];
        // num_in_neighbors_0 = num_in_neighbors;
        // num_in_neighbors_1 = num_in_neighbors;
        // num_in_neighbors_2 = num_in_neighbors;
        // turn into a void function

        pna_conv_copy_num_in_neighbors(num_in_neighbors, num_in_neighbors_0, num_in_neighbors_1, num_in_neighbors_2);

        int neighbors[MAX_NODES];

        gather_node_neighbors<
            MAX_NODES,
            MAX_EDGES,
            NUM_NODES_GUESS,
            NUM_EDGES_GUESS,
            DEGREE_GUESS>(
            node,
            num_in_neighbors_0,
            neighbors,
            neightbor_table_offsets,
            neighbor_table);

        int num_in_neighbors_clamped;
        if (num_in_neighbors_1 < 1)
        {
            num_in_neighbors_clamped = 1;
        }
        else
        {
            num_in_neighbors_clamped = num_in_neighbors_1;
        }

        T amplification_factor = m_log(T(num_in_neighbors_clamped + 1)) / pna_avg_degree_log;
        T attenuation_factor = pna_avg_degree_log / m_log(T(num_in_neighbors_clamped + 1));

        T agg_max_identity_emb[EMB_SIZE_IN];
        T agg_min_identity_emb[EMB_SIZE_IN];
        T agg_mean_identity_emb[EMB_SIZE_IN];
        T agg_std_identity_emb[EMB_SIZE_IN];

        pna_conv_agg<
            MAX_NODES,
            MAX_EDGES,
            EMB_SIZE_IN,
            EMB_SIZE_OUT,
            TRANSFORM_IN,
            TRANSFORM_OUT,
            APPLY_IN,
            APPLY_OUT,
            T,
            NUM_NODES_GUESS,
            NUM_EDGES_GUESS,
            DEGREE_GUESS,
            P_IN,
            P_OUT>(
            num_in_neighbors_2,
            neighbors,
            node_embedding_table_in,
            current_node_embedding_in_for_agg,
            agg_max_identity_emb,
            agg_min_identity_emb,
            agg_mean_identity_emb,
            agg_std_identity_emb,
            transfrom_lin_weight,
            transfrom_lin_bias);

        // const int AGG_EMB_SIZE = EMB_SIZE_IN * 4;

        // F_TYPE agg_emb[AGG_EMB_SIZE];
        // #pragma HLS array_partition variable = agg_emb dim = 1 block factor = 4
        // for (int i = 0; i < EMB_SIZE_IN; i++) {
        //     agg_emb[i] = agg_max_identity_emb[i];
        //     agg_emb[i + EMB_SIZE_IN] = agg_min_identity_emb[i];
        //     agg_emb[i + EMB_SIZE_IN * 2] = agg_mean_identity_emb[i];
        //     agg_emb[i + EMB_SIZE_IN * 3] = agg_std_identity_emb[i];
        // }

        // T agg_amplification_emb[AGG_EMB_SIZE];
        // T agg_attenuation_emb[AGG_EMB_SIZE];

        // for (int i = 0; i < EMB_SIZE_IN * 4; i++) {
        //     agg_amplification_emb[i] = amplification_factor * agg_emb[i];
        //     agg_attenuation_emb[i] = attenuation_factor * agg_emb[i];
        // }

        T agg_max_identity_0[EMB_SIZE_IN];
        T agg_min_identity_0[EMB_SIZE_IN];
        T agg_mean_identity_0[EMB_SIZE_IN];
        T agg_std_identity_0[EMB_SIZE_IN];

        T agg_max_identity_1[EMB_SIZE_IN];
        T agg_min_identity_1[EMB_SIZE_IN];
        T agg_mean_identity_1[EMB_SIZE_IN];
        T agg_std_identity_1[EMB_SIZE_IN];

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            T x_agg_max = agg_max_identity_emb[i];
            T x_agg_min = agg_min_identity_emb[i];
            T x_agg_mean = agg_mean_identity_emb[i];
            T x_agg_std = agg_std_identity_emb[i];

            agg_max_identity_0[i] = x_agg_max;
            agg_min_identity_0[i] = x_agg_min;
            agg_mean_identity_0[i] = x_agg_mean;
            agg_std_identity_0[i] = x_agg_std;

            agg_max_identity_1[i] = x_agg_max;
            agg_min_identity_1[i] = x_agg_min;
            agg_mean_identity_1[i] = x_agg_mean;
            agg_std_identity_1[i] = x_agg_std;
        }

        T agg_max_amplification[EMB_SIZE_IN];
        T agg_min_amplification[EMB_SIZE_IN];
        T agg_mean_amplification[EMB_SIZE_IN];
        T agg_std_amplification[EMB_SIZE_IN];

        T agg_max_attenuation[EMB_SIZE_IN];
        T agg_min_attenuation[EMB_SIZE_IN];
        T agg_mean_attenuation[EMB_SIZE_IN];
        T agg_std_attenuation[EMB_SIZE_IN];

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            T x_agg_max = agg_max_identity_0[i];
            T x_agg_min = agg_min_identity_0[i];
            T x_agg_mean = agg_mean_identity_0[i];
            T x_agg_std = agg_std_identity_0[i];

            agg_max_amplification[i] = amplification_factor * x_agg_max;
            agg_min_amplification[i] = amplification_factor * x_agg_min;
            agg_mean_amplification[i] = amplification_factor * x_agg_mean;
            agg_std_amplification[i] = amplification_factor * x_agg_std;

            agg_max_attenuation[i] = attenuation_factor * x_agg_max;
            agg_min_attenuation[i] = attenuation_factor * x_agg_min;
            agg_mean_attenuation[i] = attenuation_factor * x_agg_mean;
            agg_std_attenuation[i] = attenuation_factor * x_agg_std;
        }

        const int concat_size = EMB_SIZE_IN * 3 * 4 + EMB_SIZE_IN;

        T pre_apply_emb[concat_size];
        // #pragma HLS array_partition variable = pre_apply_emb dim = 1 block factor = concat_size

        pna_conv_concat<
            EMB_SIZE_IN,
            concat_size,
            T>(
            current_node_embedding_in,
            agg_max_identity_1,
            agg_min_identity_1,
            agg_mean_identity_1,
            agg_std_identity_1,
            agg_max_amplification,
            agg_min_amplification,
            agg_mean_amplification,
            agg_std_amplification,
            agg_max_attenuation,
            agg_min_attenuation,
            agg_mean_attenuation,
            agg_std_attenuation,
            pre_apply_emb);

        // self, then all the aggregations, then all the amplification aggregations, then all the attenuation aggregations
        // for (int i = 0; i < EMB_SIZE_IN; i++) {
        //     pre_apply_emb[i] = current_node_embedding_in[i];
        // }

        // for (int i = 0; i < EMB_SIZE_IN; i++) {
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 0] = agg_max_identity_emb[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 1] = agg_min_identity_emb[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 2] = agg_mean_identity_emb[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 3] = agg_std_identity_emb[i];

        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 4] = agg_max_amplification[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 5] = agg_min_amplification[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 6] = agg_mean_amplification[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 7] = agg_std_amplification[i];

        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 8] = agg_max_attenuation[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 9] = agg_min_attenuation[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 10] = agg_mean_attenuation[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + EMB_SIZE_IN * 11] = agg_std_attenuation[i];
        // }

        // add the amplification and attenuation aggregations
        // for (int i = 0; i < AGG_EMB_SIZE; i++) {
        //     pre_apply_emb[i + EMB_SIZE_IN + AGG_EMB_SIZE * 0] = agg_emb[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + AGG_EMB_SIZE * 1] = agg_amplification_emb[i];
        //     pre_apply_emb[i + EMB_SIZE_IN + AGG_EMB_SIZE * 2] = agg_attenuation_emb[i];
        // }

        // apply
        T new_node_embedding_hidden[EMB_SIZE_OUT];
        T new_node_embedding[EMB_SIZE_OUT];

        linear<concat_size, EMB_SIZE_OUT, P_IN, P_OUT, T>(pre_apply_emb, new_node_embedding_hidden, apply_lin_weight, apply_lin_bias);
        linear<EMB_SIZE_OUT, EMB_SIZE_OUT, P_OUT, P_OUT, T>(new_node_embedding_hidden, new_node_embedding, final_lin_weight, final_lin_bias);

        for (int i = 0; i < EMB_SIZE_OUT; i++)
        {
            node_embedding_table_out[node][i] = new_node_embedding[i];
        }
    }
}

// Content of called function
void gather_node_neighbors(
    int node,
    int node_in_degree,
    int node_neighbors[MAX_NODES],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES])
{

#pragma HLS INLINE off
    // #pragma HLS INLINE

    int node_offset = neightbor_table_offsets[node];
    for (int i = 0; i < node_in_degree; i++)
    {
#pragma HLS loop_tripcount min = 1 max = DEGREE_GUESS
        int current_idx = node_offset + i;
        node_neighbors[i] = neighbor_table[current_idx];
    }
}

// Content of called function
void pna_conv_concat(
    T current_node_embedding_in[EMB_SIZE_IN],
    T agg_max_identity_emb[EMB_SIZE_IN],
    T agg_min_identity_emb[EMB_SIZE_IN],
    T agg_mean_identity_emb[EMB_SIZE_IN],
    T agg_std_identity_emb[EMB_SIZE_IN],
    T agg_max_amplification[EMB_SIZE_IN],
    T agg_min_amplification[EMB_SIZE_IN],
    T agg_mean_amplification[EMB_SIZE_IN],
    T agg_std_amplification[EMB_SIZE_IN],
    T agg_max_attenuation[EMB_SIZE_IN],
    T agg_min_attenuation[EMB_SIZE_IN],
    T agg_mean_attenuation[EMB_SIZE_IN],
    T agg_std_attenuation[EMB_SIZE_IN],
    T pre_apply_emb[CONCAT_SIZE])
{
#pragma HLS INLINE off
    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        pre_apply_emb[i + EMB_SIZE_IN * 0] = current_node_embedding_in[i];

        pre_apply_emb[i + EMB_SIZE_IN * 1] = agg_max_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 2] = agg_min_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 3] = agg_mean_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 4] = agg_std_identity_emb[i];

        pre_apply_emb[i + EMB_SIZE_IN * 5] = agg_max_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 6] = agg_min_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 7] = agg_mean_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 8] = agg_std_amplification[i];

        pre_apply_emb[i + EMB_SIZE_IN * 9] = agg_max_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 10] = agg_min_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 11] = agg_mean_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 12] = agg_std_attenuation[i];
    }
}

// Content of called function
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

// Content of called function
void pna_conv_copy_num_in_neighbors(
    T &num_in_neighbors,
    T &num_in_neighbors_0,
    T &num_in_neighbors_1,
    T &num_in_neighbors_2)
{
#pragma HLS INLINE off
    num_in_neighbors_0 = num_in_neighbors;
    num_in_neighbors_1 = num_in_neighbors;
    num_in_neighbors_2 = num_in_neighbors;
}