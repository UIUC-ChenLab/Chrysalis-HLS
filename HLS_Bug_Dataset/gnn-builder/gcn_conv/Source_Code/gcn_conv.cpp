void gcn_conv(
    int num_nodes,
    int num_edges,
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T node_embedding_table_out[MAX_NODES][EMB_SIZE_OUT],
    int edge_list[MAX_EDGES][2],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES],
    int in_degree_table[MAX_NODES],
    int out_degree_table[MAX_NODES],
    T apply_lin_weight[EMB_SIZE_OUT][EMB_SIZE_IN],
    T apply_lin_bias[EMB_SIZE_OUT])
{

#pragma HLS INLINE off

    for (int node = 0; node < num_nodes; node++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS

        // #pragma HLS DATAFLOW

#pragma HLS stable variable = num_nodes
#pragma HLS stable variable = num_edges

        // #pragma HLS stable variable = edge_list
        // #pragma HLS stable variable = neightbor_table_offsets
        // #pragma HLS stable variable = neighbor_table
        // #pragma HLS stable variable = in_degree_table
        // #pragma HLS stable variable = out_degree_table

        // #pragma HLS stable variable = apply_lin_weight
        // #pragma HLS stable variable = apply_lin_bias



        T current_node_embedding_in[EMB_SIZE_IN];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            current_node_embedding_in[i] = node_embedding_table_in[node][i];
        }

        int num_in_neighbors = in_degree_table[node];
        int neighbors[MAX_NODES];

        gather_node_neighbors<
            MAX_NODES,
            MAX_EDGES,
            NUM_NODES_GUESS,
            NUM_EDGES_GUESS,
            DEGREE_GUESS>(
            node,
            num_in_neighbors,
            neighbors,
            neightbor_table_offsets,
            neighbor_table);

        T agg_embedding[EMB_SIZE_IN];
        gcn_conv_agg<
            MAX_NODES,
            MAX_EDGES,
            EMB_SIZE_IN,
            EMB_SIZE_OUT,
            T,
            NUM_NODES_GUESS,
            NUM_EDGES_GUESS,
            DEGREE_GUESS,
            P_IN,
            P_OUT>(
            current_node_embedding_in,
            num_in_neighbors,
            neighbors,
            node_embedding_table_in,
            in_degree_table,
            agg_embedding);

        // compute new node embedding
        T new_node_embedding[EMB_SIZE_OUT];
        linear<EMB_SIZE_IN, EMB_SIZE_OUT, P_IN, P_OUT, T>(agg_embedding, new_node_embedding, apply_lin_weight, apply_lin_bias);

        // node_embedding_table_out[node] = new_node_embedding;
        for (int i = 0; i < EMB_SIZE_OUT; i++)
        {
            node_embedding_table_out[node][i] = new_node_embedding[i];
        }
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
void gcn_conv_agg(
    T current_node_embedding_in[EMB_SIZE_IN],
    int num_in_neighbors,
    int neighbors[MAX_NODES],
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    int in_degree_table[MAX_NODES],
    T agg_embedding[EMB_SIZE_IN])
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

        int in_degree_neighbor = in_degree_table[neighbor_id];
        int in_degree_node = num_in_neighbors;

        T d_i_prime = T(1.0) + T(in_degree_node);
        T d_j_prime = T(1.0) + T(in_degree_neighbor);

        T degree_scaling_factor = m_recip(T(m_sqrt(d_i_prime * d_j_prime)));

        T neighbor_emb_transformed[EMB_SIZE_IN];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_emb_transformed[i] = neighbor_emb[i] * degree_scaling_factor;
        }

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            sum_incremental_update(neighbor_emb_aggregation_sum[i], neighbor_emb_transformed[i]);
        }
    }

    T d_i_prime = T(1.0) + num_in_neighbors;
    T degree_scaling_factor_self = m_recip(m_sqrt(d_i_prime * d_i_prime));

    T self_emb_transformed[EMB_SIZE_IN];
    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        self_emb_transformed[i] = current_node_embedding_in[i] * degree_scaling_factor_self;
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        sum_incremental_update(neighbor_emb_aggregation_sum[i], self_emb_transformed[i]);
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
void sum_incremental_update(sum_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
}

// Content of called function
void sum_incremental_finalize(sum_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}