initialize_edge_weights_table(typename CONFIG_T::edge_weight_t edge_weights_table[]) {
    typedef ap_uint<CONFIG_T::distance_width> index_t;

    unsigned const table_size = (1 << CONFIG_T::distance_width);

    index_t index;
    typename CONFIG_T::distance_t distance;

    // edge_weight_t is ap_ufixed with 0 iwidth -> let index 0 be a saturated version of 1
    edge_weights_table[0] = ap_ufixed<CONFIG_T::edge_weight_t::width, 0, AP_TRN, AP_SAT>(1.);

    for (unsigned iw = 1; iw < table_size; ++iw) {
        index = iw;
        distance.range(CONFIG_T::distance_width - 1, 0) = index.range(CONFIG_T::distance_width - 1, 0);
        edge_weights_table[iw] = hls::exp(-distance * distance);
    }
}