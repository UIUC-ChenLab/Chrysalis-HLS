void pointwise_conv_1d_cl(data_T data[CONFIG_T::in_width * CONFIG_T::n_chan],
                          res_T res[CONFIG_T::out_width * CONFIG_T::n_filt],
                          typename CONFIG_T::weight_t weights[CONFIG_T::n_chan * CONFIG_T::n_filt],
                          typename CONFIG_T::bias_t biases[CONFIG_T::n_filt]) {
    assert(CONFIG_T::filt_width == 1);

    #pragma HLS INLINE region

    // Nothing special to be done for io_parallel implementation
    if (CONFIG_T::strategy == nnet::latency) {
        conv_1d_latency_cl<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    } else {
        conv_1d_resource_cl<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    }
}