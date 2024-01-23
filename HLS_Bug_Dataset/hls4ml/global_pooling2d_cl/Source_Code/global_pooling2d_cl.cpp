void global_pooling2d_cl(data_T data[CONFIG_T::in_height * CONFIG_T::in_width * CONFIG_T::n_filt],
                         res_T res[CONFIG_T::n_filt]) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::pad_top == 0 && CONFIG_T::pad_bottom == 0);
    assert(CONFIG_T::pool_width == CONFIG_T::stride_width);
    assert(CONFIG_T::pool_height == CONFIG_T::stride_height);

    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    const int limit = pool_op_limit<CONFIG_T>();
    #pragma HLS ALLOCATION instances=pool_op limit=limit function

FiltLoop:
    for (int filt = 0; filt < CONFIG_T::n_filt; filt++) {
        data_T pool[CONFIG_T::in_height * CONFIG_T::in_width];

    InputLoop:
        for (int i = 0; i < CONFIG_T::in_height * CONFIG_T::in_width; i++) {
            pool[i] = data[i * CONFIG_T::n_filt + filt];
        }

        res[filt] = static_cast<res_T>(pool_op<data_T, CONFIG_T::in_height * CONFIG_T::in_width, CONFIG_T::pool_op>(pool));
    }
}