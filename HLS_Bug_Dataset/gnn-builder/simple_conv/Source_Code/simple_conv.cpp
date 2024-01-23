void simple_conv(
    int num_nodes,
    int num_edges,
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T node_embedding_table_out[MAX_NODES][EMB_SIZE_OUT],
    int edge_list[MAX_EDGES][2],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES],
    int in_degree_table[MAX_NODES],
    int out_degree_table[MAX_NODES])
{

#pragma HLS INLINE off

    for (int node = 0; node < num_nodes; node++)
    {
#pragma HLS loop_tripcount min = 0 max = NUM_NODES_GUESS

#pragma HLS DATAFLOW
#pragma HLS stable variable = edge_list
#pragma HLS stable variable = neightbor_table_offsets
#pragma HLS stable variable = neighbor_table
#pragma HLS stable variable = in_degree_table
#pragma HLS stable variable = out_degree_table

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
        simple_conv_agg<
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

        for (int i = 0; i < EMB_SIZE_OUT; i++)
        {
            node_embedding_table_out[node][i] = agg_embedding[i];
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
void simple_conv_agg(
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
#pragma HLS loop_tripcount min = 0 max = DEGREE_GUESS

        T neighbor_emb[EMB_SIZE_IN];

        int neighbor_id = neighbors[neighbor];
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            neighbor_emb[i] = node_embedding_table_in[neighbor_id][i];
        }

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            sum_incremental_update(neighbor_emb_aggregation_sum[i], neighbor_emb[i]);
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