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