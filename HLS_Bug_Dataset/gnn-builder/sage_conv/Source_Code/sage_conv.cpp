void sage_conv(
    int num_nodes,
    int num_edges,
    T node_embedding_table_in[MAX_NODES][EMB_SIZE_IN],
    T node_embedding_table_out[MAX_NODES][EMB_SIZE_OUT],
    int edge_list[MAX_EDGES][2],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES],
    int in_degree_table[MAX_NODES],
    int out_degree_table[MAX_NODES],
    T neighbor_lin_weight[EMB_SIZE_OUT][EMB_SIZE_IN],
    T neighbor_lin_bias[EMB_SIZE_OUT],
    T self_lin_weight[EMB_SIZE_OUT][EMB_SIZE_IN])
{
#pragma HLS stable variable = num_nodes
#pragma HLS stable variable = num_edges

#pragma HLS DATAFLOW
    T node_embedding_table_in_0[MAX_NODES][EMB_SIZE_IN];
    T node_embedding_table_in_1[MAX_NODES][EMB_SIZE_IN];

    for (int node = 0; node < num_nodes; node++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS
        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
#pragma HLS loop_tripcount min = 1 max = EMB_SIZE_IN
            T x_in = node_embedding_table_in[node][i];
            node_embedding_table_in_0[node][i] = x_in;
            node_embedding_table_in_1[node][i] = x_in;
        }
    }

#pragma HLS INLINE off

    for (int node = 0; node < num_nodes; node++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS

#pragma HLS DATAFLOW
#pragma HLS stable variable = num_nodes
#pragma HLS stable variable = num_edges

        // #pragma HLS stable variable = edge_list
        // #pragma HLS stable variable = neightbor_table_offsets
        // #pragma HLS stable variable = neighbor_table
        // #pragma HLS stable variable = in_degree_table
        // #pragma HLS stable variable = out_degree_table

        // #pragma HLS stable variable = neighbor_lin_weight
        // #pragma HLS stable variable = neighbor_lin_bias

        // #pragma HLS stable variable = self_lin_weight

        T current_node_embedding_in[EMB_SIZE_IN];

        for (int i = 0; i < EMB_SIZE_IN; i++)
        {
            current_node_embedding_in[i] = node_embedding_table_in_0[node][i];
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

        T agg_emb[EMB_SIZE_IN];
        sage_conv_agg<
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
            num_in_neighbors,
            neighbors,
            node_embedding_table_in_1,
            agg_emb);

        // transform the aggregation embedding
        T agg_emb_transformed[EMB_SIZE_OUT];
        linear<EMB_SIZE_IN, EMB_SIZE_OUT, P_IN, P_OUT, T>(agg_emb, agg_emb_transformed, neighbor_lin_weight, neighbor_lin_bias);

        // transform the current node embedding
        T self_emb_transformed[EMB_SIZE_OUT];
        T init = T(0.0);
        T self_lin_bias[EMB_SIZE_OUT];
        for (int i = 0; i < EMB_SIZE_OUT; i++)
        {
            self_lin_bias[i] = init;
        }
        linear<EMB_SIZE_IN, EMB_SIZE_OUT, P_IN, P_OUT, T>(current_node_embedding_in, self_emb_transformed, self_lin_weight, self_lin_bias);

        // add the two embeddings
        T new_node_embedding[EMB_SIZE_OUT];
        for (int i = 0; i < EMB_SIZE_OUT; i++)
        {
            new_node_embedding[i] = agg_emb_transformed[i] + self_emb_transformed[i];
        }

        // write to node_embedding_table_out
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