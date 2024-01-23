compute_vertex_output(arrays_T const &arrays, unsigned iv,
                      typename CONFIG_T::aggr_t const output_base[CONFIG_T::n_out_features * CONFIG_T::n_aggregators],
                      res_setter_T &res_setter) {
    #pragma HLS INLINE

    typename arrays_T::edge_weight_t edge_weights[CONFIG_T::n_aggregators];
    #pragma HLS ARRAY_PARTITION variable=edge_weights complete

Aggregators1:
    for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
        unsigned const iva = iv * CONFIG_T::n_aggregators + ia;

        edge_weights[ia] = arrays.edge_weights[iva];
    }

OutFeatures:
    for (unsigned io = 0; io < CONFIG_T::n_out_features; ++io) {
        typename res_setter_T::res_t acc = CONFIG_T::output_transform_biases[io];

    Aggregators2:
        for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
            unsigned const ioa = io * CONFIG_T::n_aggregators + ia;

            typename res_setter_T::res_t incr = edge_weights[ia] * output_base[ioa];
            acc += incr;
        }

        res_setter.set(iv, io, acc);
    }
}