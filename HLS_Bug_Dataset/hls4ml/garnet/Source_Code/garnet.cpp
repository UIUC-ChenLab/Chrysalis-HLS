garnet(data_T const data[CONFIG_T::n_vertices * CONFIG_T::n_in_features], nvtx_T const nvtx[1],
       res_T res[CONFIG_T::n_vertices * CONFIG_T::n_out_features]) {
    #pragma HLS DATAFLOW

    garnet_utils::WeightsAndMeans<CONFIG_T> arrays;

    garnet_utils::aggregate<CONFIG_T>(data, nvtx[0], arrays);

    garnet_utils::distribute<CONFIG_T>(nvtx[0], arrays, res);
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
void distribute(nvtx_T const nvtx, arrays_T const &arrays, res_T res[CONFIG_T::n_vertices * CONFIG_T::n_out_features]) {
    OutputResSetter<CONFIG_T, res_T> res_setter(res);

    typename CONFIG_T::aggr_t output_base[CONFIG_T::n_out_features * CONFIG_T::n_aggregators];
    #pragma HLS ARRAY_PARTITION variable=output_base complete

    compute_output_base<CONFIG_T>(arrays, output_base);

    unsigned const unroll_factor = CONFIG_T::n_vertices >> CONFIG_T::log2_reuse_factor;

VerticesOuter:
    for (unsigned ivv = 0; ivv < CONFIG_T::reuse_factor; ++ivv) {
        #pragma HLS PIPELINE

        if (ivv * unroll_factor >= nvtx)
            break;

    VerticesInner:
        for (unsigned ir = 0; ir < unroll_factor; ++ir) {
            unsigned iv = ivv * unroll_factor + ir;

            if (iv == nvtx)
                break;

            compute_vertex_output<CONFIG_T>(arrays, iv, output_base, res_setter);
        }
    }
}

// Content of called function
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