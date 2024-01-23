void sage_conv_agg(
    int num_in_neighbors,
    int neighbors[MAX_NODES],
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T agg_embedding[EMB_SIZE_IN])
{
#pragma HLS INLINE off

    mean_incremental_data<T> neighbor_emb_aggregation_mean[EMB_SIZE_IN];

    for (int neighbor = 0; neighbor < num_in_neighbors; neighbor++)
    {
#pragma HLS loop_tripcount min = 1 max = DEGREE_GUESS

        T neighbor_emb[EMB_SIZE_IN];

        int neighbor_id = neighbors[neighbor];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_emb[i] = node_embedding_table_in[neighbor_id][i];
        }

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            mean_incremental_update(neighbor_emb_aggregation_mean[i], neighbor_emb[i]);
        }
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        mean_incremental_finalize(neighbor_emb_aggregation_mean[i]);
    }

    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        agg_embedding[i] = neighbor_emb_aggregation_mean[i].mean;
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