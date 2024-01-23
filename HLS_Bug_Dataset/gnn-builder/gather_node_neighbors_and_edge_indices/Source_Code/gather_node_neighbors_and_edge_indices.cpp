void gather_node_neighbors_and_edge_indices(
    int node,
    int node_in_degree,
    int node_neighbors[MAX_NODES],
    int node_edge_indices[MAX_NODES],
    int neightbor_table_offsets[MAX_NODES],
    int neighbor_table[MAX_EDGES],
    int edge_index_table[MAX_EDGES])
{
    int node_offset = neightbor_table_offsets[node];
    for (int i = 0; i < node_in_degree; i++)
    {
#pragma HLS loop_tripcount min = 1 max = DEGREE_GUESS
        int current_idx = node_offset + i;
        node_neighbors[i] = neighbor_table[current_idx];
        node_edge_indices[i] = edge_index_table[current_idx];
    }
}