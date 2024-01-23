garnet_ref(data_T const data[CONFIG_T::n_vertices * CONFIG_T::n_in_features], nvtx_T const nvtx[1],
           res_T res[CONFIG_T::n_out_features]) {
    typename CONFIG_T::aggr_t vertex_res[CONFIG_T::n_vertices * CONFIG_T::n_out_features];

    garnet_ref<CONFIG_T>(data, nvtx, vertex_res);

    for (unsigned io = 0; io < CONFIG_T::n_out_features; ++io) {
        typename CONFIG_T::aggr_t acc = 0.;

        for (unsigned iv = 0; iv < CONFIG_T::n_vertices; ++iv) {
            if (iv == nvtx[0])
                break;

            unsigned const ivo = iv * CONFIG_T::n_out_features + io;

            acc += vertex_res[ivo];
        }

        if (CONFIG_T::mean_by_nvert)
            acc /= nvtx[0];
        else {
            // Not using right shift in case aggr_t is float or double
            acc /= CONFIG_T::n_vertices;
        }

        res[io] = acc;
    }
}