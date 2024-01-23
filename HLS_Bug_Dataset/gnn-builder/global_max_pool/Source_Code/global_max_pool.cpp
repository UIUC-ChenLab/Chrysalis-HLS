void global_max_pool(int num_nodes,
                     int num_edges,
                     T node_embedding_table[MAX_NODES][EMB_SIZE],
                     T pooled_embedding[EMB_SIZE])
{
#pragma HLS INLINE off

    max_incremental_data<T> max_agg[EMB_SIZE];
    for (int i = 0; i < num_nodes; i++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS
        for (int j = 0; j < EMB_SIZE; j++)
        {
            max_incremental_update(max_agg[j], node_embedding_table[i][j]);
        }
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        max_incremental_finalize(max_agg[i]);
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        pooled_embedding[i] = max_agg[i].max;
    }
}

// Content of called function
void max_incremental_finalize(max_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
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