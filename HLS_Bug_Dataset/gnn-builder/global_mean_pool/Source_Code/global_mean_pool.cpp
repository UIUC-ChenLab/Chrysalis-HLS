void global_mean_pool(int num_nodes,
                      int num_edges,
                      T node_embedding_table[MAX_NODES][EMB_SIZE],
                      T pooled_embedding[EMB_SIZE])
{
#pragma HLS INLINE off

    mean_incremental_data<T> mean_agg[EMB_SIZE];
    for (int i = 0; i < num_nodes; i++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS
        for (int j = 0; j < EMB_SIZE; j++)
        {
            mean_incremental_update(mean_agg[j], node_embedding_table[i][j]);
        }
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        mean_incremental_finalize(mean_agg[i]);
    }
    for (int i = 0; i < EMB_SIZE; i++)
    {
        pooled_embedding[i] = mean_agg[i].mean;
    }
}

// Content of called function
void mean_incremental_finalize(mean_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}

// Content of called function
void mean_incremental_update(mean_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
    data.count++;
    data.mean = data.sum / T(data.count);
}