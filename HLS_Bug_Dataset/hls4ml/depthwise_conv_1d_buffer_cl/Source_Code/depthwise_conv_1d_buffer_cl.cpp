void depthwise_conv_1d_buffer_cl(hls::stream<data_T> &data, hls::stream<res_T> &res,
                                 typename CONFIG_T::weight_t weights[CONFIG_T::filt_width * CONFIG_T::n_chan],
                                 typename CONFIG_T::bias_t biases[CONFIG_T::n_chan]) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);

ReadInputWidth:
    for (unsigned i_iw = 0; i_iw < CONFIG_T::in_width; i_iw++) {
        #pragma HLS LOOP_FLATTEN
        if (CONFIG_T::strategy == nnet::latency) {
            #pragma HLS PIPELINE II=CONFIG_T::reuse_factor
        }
        compute_depthwise_output_buffer_1d<data_T, res_T, CONFIG_T>(data.read(), res, weights, biases);
    }
}