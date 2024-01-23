void normalize_ternary_tanh(data_T data[CONFIG_T::n_in], ap_int<2> res[CONFIG_T::n_in],
                            data_T threshold_hi[CONFIG_T::n_scale_bias], data_T threshold_lo[CONFIG_T::n_scale_bias]) {
    #pragma HLS PIPELINE
    #pragma HLS ARRAY_PARTITION variable=res complete

    data_T datareg;
    ap_int<2> cache;
    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
        datareg = data[ii];
        int norm_index = CONFIG_T::n_filt == -1 ? ii : ii % CONFIG_T::n_filt;
        if (datareg > threshold_hi[norm_index])
            cache = 1;
        else if (datareg <= threshold_lo[norm_index])
            cache = -1;
        else
            cache = 0;

        res[ii] = cache;
    }
}