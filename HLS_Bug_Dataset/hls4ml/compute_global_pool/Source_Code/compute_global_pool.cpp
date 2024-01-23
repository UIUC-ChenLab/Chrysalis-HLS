void compute_global_pool(const data_T &in_elem, typename CONFIG_T::accum_t data_window[CONFIG_T::n_filt]) {
PoolFilt:
    for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
        #pragma HLS UNROLL

        typename CONFIG_T::accum_t data_pack[data_T::size / CONFIG_T::n_filt];
        #pragma HLS ARRAY_PARTITION variable=data_pack complete dim=0

    PixelLoop:
        for (unsigned p = 0; p < data_T::size / CONFIG_T::n_filt; p++) {
            #pragma HLS UNROLL
            data_pack[p] = in_elem[p * CONFIG_T::n_filt + c];
        }
        data_window[c] = reduce_global_pool<typename CONFIG_T::accum_t, data_T::size / CONFIG_T::n_filt, CONFIG_T>(
            data_window[c], data_pack);
    }
}