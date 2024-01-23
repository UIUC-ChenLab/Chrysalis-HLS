void global_add_pool(int num_nodes,
                     int num_edges,
                     T node_embedding_table[MAX_NODES][EMB_SIZE],
                     T pooled_embedding[EMB_SIZE])
{
#pragma HLS INLINE off

    sum_incremental_data<T> sum_agg[EMB_SIZE];
    for (int i = 0; i < num_nodes; i++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS
        for (int j = 0; j < EMB_SIZE; j++)
        {
            sum_incremental_update(sum_agg[j], node_embedding_table[i][j]);
        }
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        sum_incremental_finalize(sum_agg[i]);
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        pooled_embedding[i] = sum_agg[i].sum;
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