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