void conv_1d_encoded_cl(hls::stream<data_T> &data, hls::stream<res_T> &res,
                        typename CONFIG_T::weight_t weights[CONFIG_T::filt_width * CONFIG_T::n_chan * CONFIG_T::n_filt],
                        typename CONFIG_T::bias_t biases[CONFIG_T::n_filt]) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);

    hls::stream<typename data_T::value_type> data_window[CONFIG_T::filt_width * CONFIG_T::n_chan];
    const int win_depth = CONFIG_T::out_width;
    for (unsigned i_out = 0; i_out < CONFIG_T::filt_width * CONFIG_T::n_chan; i_out++) {
        #pragma HLS STREAM variable=data_window[i_out] depth=win_depth
    }

    #pragma HLS ARRAY_PARTITION variable=CONFIG_T::pixels complete

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)
    unsigned outputs_ready = 0;

    ap_uint<CONFIG_T::filt_width> pixel_idx[data_T::size / CONFIG_T::n_chan];
    #pragma HLS ARRAY_PARTITION variable=pixel_idx complete

ReadInputWidth:
    for (unsigned i_iw = 0; i_iw < CONFIG_T::in_width / (data_T::size / CONFIG_T::n_chan); i_iw++) {
        #pragma HLS LOOP_FLATTEN
        if (CONFIG_T::strategy == nnet::latency && data_T::size / CONFIG_T::n_chan == 1) {
            #pragma HLS PIPELINE II=CONFIG_T::reuse_factor
        }
        compute_scaled_indices_1d<data_T, CONFIG_T>(i_iw, pixel_idx);
        compute_output_encoded<data_T, res_T, CONFIG_T>(data.read(), data_window, res, res_pack, outputs_ready, weights,
                                                        biases, pixel_idx);
    }
}

// Content of called function
void compute_scaled_indices_1d(const unsigned w_idx, ap_uint<CONFIG_T::filt_width> *pixel_idx) {
    unsigned wp_idx = w_idx * (data_T::size / CONFIG_T::n_chan);

ComputeIndex:
    for (unsigned p = 0; p < data_T::size / CONFIG_T::n_chan; p++) {
        #pragma HLS UNROLL
        unsigned sw_idx =
            CONFIG_T::template scale_index<CONFIG_T::filt_width, CONFIG_T::stride_width, CONFIG_T::in_width>::scale_index(
                wp_idx + p);
        pixel_idx[p] = CONFIG_T::pixels[sw_idx];
    }
}