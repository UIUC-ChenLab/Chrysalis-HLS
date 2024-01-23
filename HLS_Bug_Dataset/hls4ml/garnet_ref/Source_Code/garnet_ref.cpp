garnet_ref(data_T const data[CONFIG_T::n_vertices * CONFIG_T::n_in_features], nvtx_T const nvtx[1],
           res_T res[CONFIG_T::n_vertices * CONFIG_T::n_out_features]) {
    typename CONFIG_T::edge_weight_t edge_weights[CONFIG_T::n_vertices * CONFIG_T::n_aggregators];
    typename CONFIG_T::aggr_t propagated_features[CONFIG_T::n_vertices * CONFIG_T::n_propagate];

    for (unsigned iv = 0; iv < CONFIG_T::n_vertices; ++iv) {
        if (iv == nvtx[0])
            break;

        for (unsigned ip = 0; ip < CONFIG_T::n_propagate; ++ip) {
            unsigned const ivp = iv * CONFIG_T::n_propagate + ip;

            propagated_features[ivp] = CONFIG_T::input_transform_biases[ip];

            for (unsigned ix = 0; ix < CONFIG_T::n_in_features; ++ix) {
                unsigned const ivx = iv * CONFIG_T::n_in_features + ix;
                unsigned const ipx = ip * CONFIG_T::n_in_features + ix;

                propagated_features[ivp] += data[ivx] * CONFIG_T::input_transform_weights[ipx];
            }
        }

        for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
            unsigned const iva = iv * CONFIG_T::n_aggregators + ia;

            typename CONFIG_T::aggr_t distance = CONFIG_T::aggregator_distance_biases[ia];

            for (unsigned ix = 0; ix < CONFIG_T::n_in_features; ++ix) {
                unsigned const ivx = iv * CONFIG_T::n_in_features + ix;
                unsigned const iax = ia * CONFIG_T::n_in_features + ix;

                distance += data[ivx] * CONFIG_T::aggregator_distance_weights[iax];
            }

            edge_weights[iva] = garnet_utils::compute_edge_weight<CONFIG_T>(distance);
        }
    }

    typename CONFIG_T::aggr_t aggregated_features[CONFIG_T::n_aggregators * CONFIG_T::n_propagate];

    for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
        for (unsigned ip = 0; ip < CONFIG_T::n_propagate; ++ip) {
            unsigned const iap = ia * CONFIG_T::n_propagate + ip;

            aggregated_features[iap] = 0.;

            for (unsigned iv = 0; iv < CONFIG_T::n_vertices; ++iv) {
                if (iv == nvtx[0])
                    break;

                unsigned const iva = iv * CONFIG_T::n_aggregators + ia;
                unsigned const ivp = iv * CONFIG_T::n_propagate + ip;

                aggregated_features[iap] += edge_weights[iva] * propagated_features[ivp];
            }
        }
    }

    for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
        for (unsigned ip = 0; ip < CONFIG_T::n_propagate; ++ip) {
            unsigned const iap = ia * CONFIG_T::n_propagate + ip;

            if (CONFIG_T::mean_by_nvert)
                aggregated_features[iap] /= nvtx[0];
            else {
                // Not using right shift in case aggr_t is float or double
                aggregated_features[iap] /= CONFIG_T::n_vertices;
            }
        }
    }

    for (unsigned iv = 0; iv < CONFIG_T::n_vertices; ++iv) {
        if (iv == nvtx[0])
            break;

        for (unsigned io = 0; io < CONFIG_T::n_out_features; ++io) {
            unsigned const ivo = iv * CONFIG_T::n_out_features + io;

            typename CONFIG_T::aggr_t acc = CONFIG_T::output_transform_biases[io];

            for (unsigned ia = 0; ia < CONFIG_T::n_aggregators; ++ia) {
                unsigned const iva = iv * CONFIG_T::n_aggregators + ia;
                unsigned const ioa = io * CONFIG_T::n_aggregators + ia;

                typename CONFIG_T::aggr_t aggr = 0.;

                for (unsigned ip = 0; ip < CONFIG_T::n_propagate; ++ip) {
                    unsigned const iap = ia * CONFIG_T::n_propagate + ip;
                    unsigned const ioap = ioa * CONFIG_T::n_propagate + ip;

                    aggr += CONFIG_T::output_transform_weights[ioap] * aggregated_features[iap];
                }

                acc += edge_weights[iva] * aggr;
            }

            res[ivo] = acc;
        }
    }
}