void compute_degree_tables(int edge_list[MAX_EDGES][2],
                           int in_degree_table[MAX_NODES],
                           int out_degree_table[MAX_NODES],
                           int num_nodes,
                           int num_edges)
{
#pragma HLS INLINE off

    //  edge list is a COO format
    //  edge_list[i][0] = source node
    //  edge_list[i][1] = destination node

    for (int i = 0; i < num_nodes; i++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_NODES_GUESS

        in_degree_table[i] = 0;
        out_degree_table[i] = 0;
    }

    for (int i = 0; i < num_edges; i++)
    {
#pragma HLS loop_tripcount min = 1 max = NUM_EDGES_GUESS
#pragma HLS PIPELINE off

        int source = edge_list[i][0];
        int dest = edge_list[i][1];

        in_degree_table[dest]++;
        out_degree_table[source]++;
    }
}