void fill_mult(typename CONFIG_T::index_t index, typename CONFIG_T::accum_t mult[CONFIG_T::n_out],
               typename CONFIG_T::accum_t weight) {
    for (unsigned k = 0; k < CONFIG_T::n_out; k++) {
        #pragma HLS UNROLL
        if (k == index)
            mult[k] += weight;
    }
}