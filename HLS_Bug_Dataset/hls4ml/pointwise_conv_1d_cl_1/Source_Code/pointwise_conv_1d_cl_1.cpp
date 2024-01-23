void pointwise_conv_1d_cl(hls::stream<data_T> &data, hls::stream<res_T> &res,
                          typename CONFIG_T::weight_t weights[CONFIG_T::n_chan * CONFIG_T::n_filt],
                          typename CONFIG_T::bias_t biases[CONFIG_T::n_filt]) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::filt_width == 1);

    #pragma HLS ARRAY_PARTITION variable=weights complete
    #pragma HLS ARRAY_PARTITION variable=biases complete

ReadInputWidth:
    for (unsigned i_iw = 0; i_iw < CONFIG_T::in_width / (data_T::size / CONFIG_T::n_chan); i_iw++) {
        if (CONFIG_T::strategy == nnet::latency && data_T::size / CONFIG_T::n_chan == 1) {
            #pragma HLS PIPELINE II=CONFIG_T::reuse_factor
        }
        if (i_iw % CONFIG_T::stride_width == 0) {
            pointwise_mult_buffer<data_T, res_T, CONFIG_T>(data.read(), res, weights, biases);
        } else {
            data.read();
        }
    }
}