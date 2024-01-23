get_edge_weight(typename CONFIG_T::distance_t distance, typename CONFIG_T::edge_weight_t edge_weights_table[]) {
    typedef ap_uint<CONFIG_T::distance_width> index_t;

    index_t index(distance.range(CONFIG_T::distance_width - 1, 0));

    return edge_weights_table[index];
}