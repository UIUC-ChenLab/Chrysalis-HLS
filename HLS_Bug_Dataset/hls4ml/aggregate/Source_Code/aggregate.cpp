void aggregate(data_T const data[CONFIG_T::n_vertices * CONFIG_T::n_in_features], nvtx_T const nvtx, arrays_T &arrays) {
    InputDataGetter<CONFIG_T, data_T> data_getter(data);

    unsigned const unroll_factor = CONFIG_T::n_vertices >> CONFIG_T::log2_reuse_factor;

    Means<CONFIG_T, typename CONFIG_T::edge_weight_aggr_t> means_accum;

VerticesOuter:
    for (unsigned ivv = 0; ivv < CONFIG_T::reuse_factor; ++ivv) {
        #pragma HLS PIPELINE

        if (ivv * unroll_factor >= nvtx)
            break;

        Means<CONFIG_T, typename CONFIG_T::edge_weight_aggr_t> means_local;

    VerticesInner:
        for (unsigned ir = 0; ir < unroll_factor; ++ir) {
            unsigned iv = ivv * unroll_factor + ir;

            if (iv == nvtx)
                break;

            compute_weights_aggregates<CONFIG_T>(data_getter, iv, means_local, arrays);
        }

        means_accum.add_means_normalized(means_local);
    }

    arrays.set_means_normalized(nvtx, means_accum);
}