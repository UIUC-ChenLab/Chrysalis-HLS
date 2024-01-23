void normalize(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_in],
               typename CONFIG_T::scale_t scale[CONFIG_T::n_scale_bias],
               typename CONFIG_T::bias_t bias[CONFIG_T::n_scale_bias]) {
    data_T cache;

    // Use a function_instantiate in case it helps to explicitly optimize unchanging weights/biases
    #pragma HLS function_instantiate variable=scale,bias

    // For parallel inputs:
    //   - completely partition arrays -- target fabric
    //   - if we have an unroll factor, limit number of multipliers
    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    // #pragma HLS ARRAY_PARTITION variable=weights complete // remove this line for now, it breaks compression sometimes
    #pragma HLS ARRAY_PARTITION variable=scale complete
    #pragma HLS ARRAY_PARTITION variable=bias complete

    #pragma HLS ALLOCATION operation instances=mul limit=CONFIG_T::multiplier_limit

// Calcuate result
Result:
    for (int ires = 0; ires < CONFIG_T::n_in; ires++) {
        if (CONFIG_T::n_filt == -1) {
            res[ires] = CONFIG_T::template product<data_T, typename CONFIG_T::scale_t>::product(data[ires], scale[ires]) +
                        bias[ires];
        } else {
            int norm_index = ires % CONFIG_T::n_filt;
            res[ires] =
                CONFIG_T::template product<data_T, typename CONFIG_T::scale_t>::product(data[ires], scale[norm_index]) +
                bias[norm_index];
        }
    }
}