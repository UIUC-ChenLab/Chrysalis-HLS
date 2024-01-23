void global_pooling1d_cl(data_T data[CONFIG_T::n_in * CONFIG_T::n_filt], res_T res[CONFIG_T::n_filt]) {
    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::pool_width == CONFIG_T::stride_width);

    // TODO partition the arrays according to the reuse factor
    const int limit = pool_op_limit_1d<CONFIG_T>();
    #pragma HLS ALLOCATION function instances=CONFIG_T::pool_op limit=limit

    for (int ff = 0; ff < CONFIG_T::n_filt; ff++) {
        data_T pool[CONFIG_T::n_in];
        #pragma HLS ARRAY_PARTITION variable=pool complete dim=0
        for (int jj = 0; jj < CONFIG_T::n_in; jj++) {
            pool[jj] = data[jj * CONFIG_T::n_filt + ff];
        }
        // do the pooling
        res[ff] = pool_op<data_T, CONFIG_T::n_in, CONFIG_T::pool_op>(pool);
    }
}