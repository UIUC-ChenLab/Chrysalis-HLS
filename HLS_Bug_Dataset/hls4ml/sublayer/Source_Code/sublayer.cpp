sublayer(nvtx_T const nvtx, prev_arrays_T const &prev_arrays, last_arrays_T &last_arrays) {
    #pragma HLS INLINE

    distribute_aggregate<prev_layer_t, current_layer_t>(nvtx, prev_arrays, last_arrays);
}

// Content of called function
void distribute_aggregate(nvtx_T const nvtx, prev_arrays_T const &prev_arrays, current_arrays_T &current_arrays) {
    typedef typename prev_layer_t::output_t data_T;

    typename prev_layer_t::aggr_t prev_output_base[prev_layer_t::n_out_features * prev_layer_t::n_aggregators];
    #pragma HLS ARRAY_PARTITION variable=prev_output_base complete

    compute_output_base<prev_layer_t>(prev_arrays, prev_output_base);

    unsigned const unroll_factor = current_layer_t::n_vertices >> current_layer_t::log2_reuse_factor;

    Means<current_layer_t, typename current_layer_t::edge_weight_aggr_t> means_accum;

VerticesOuter:
    for (unsigned ivv = 0; ivv < current_layer_t::reuse_factor; ++ivv) {
        #pragma HLS PIPELINE

        if (ivv * unroll_factor >= nvtx)
            break;

        Means<current_layer_t, typename current_layer_t::edge_weight_aggr_t> means_local;

    VerticesInner:
        for (unsigned ir = 0; ir < unroll_factor; ++ir) {
            unsigned iv = ivv * unroll_factor + ir;

            if (iv == nvtx)
                break;

            data_T data[prev_layer_t::n_out_features];
            #pragma HLS ARRAY_PARTITION variable=data complete

            SingleVertexResSetter<prev_layer_t, data_T> res_setter(data);

            compute_vertex_output<prev_layer_t>(prev_arrays, iv, prev_output_base, res_setter);

            SingleVertexDataGetter<current_layer_t, data_T> data_getter(data);

            compute_weights_aggregates<current_layer_t>(data_getter, iv, means_local, current_arrays);
        }

        means_accum.add_means_normalized(means_local);
    }

    current_arrays.set_means_normalized(nvtx, means_accum);
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