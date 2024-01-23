garnet(data_T const data[CONFIG_T::n_vertices * CONFIG_T::n_in_features], nvtx_T const nvtx[1],
       res_T res[CONFIG_T::n_out_features]) {
    #pragma HLS DATAFLOW

    garnet_utils::Means<CONFIG_T> arrays;

    garnet_utils::aggregate<CONFIG_T>(data, nvtx[0], arrays);

    garnet_utils::OutputBiasNormalizer<CONFIG_T, nvtx_T> normalize_bias(nvtx[0]);

    garnet_utils::set_output<CONFIG_T>(normalize_bias, arrays, res);
}

// Content of called function
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

// Content of called function
void set_output(output_biases_T const &output_transform_biases, arrays_T const &arrays,
                res_T res[CONFIG_T::n_out_features]) {
    #pragma HLS PIPELINE

OutFeatures:
    for (unsigned io = 0; io < CONFIG_T::n_out_features; ++io) {
        res_T acc = output_transform_biases.output_biases[io];

    Aggregators:
        for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
            typename CONFIG_T::aggr_t aggr = compute_output_base_core<CONFIG_T>(arrays, io, ia);

            acc += arrays.edge_weight_mean[ia] * aggr;
        }

        res[io] = acc;
    }
}