void pooling2d_encoded_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::pad_top == 0 && CONFIG_T::pad_bottom == 0 && CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::pool_height == CONFIG_T::stride_height && CONFIG_T::pool_width == CONFIG_T::stride_width);

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)
    unsigned outputs_ready = 0;

    hls::stream<typename data_T::value_type> data_window[CONFIG_T::pool_height * CONFIG_T::pool_width * CONFIG_T::n_filt];
    constexpr int win_depth = CONFIG_T::pool_height * CONFIG_T::out_width;
    for (unsigned i_out = 0; i_out < CONFIG_T::pool_height * CONFIG_T::pool_width * CONFIG_T::n_filt; i_out++) {
        #pragma HLS STREAM variable=data_window[i_out] depth=win_depth
    }

    constexpr int pack_factor = data_T::size / CONFIG_T::n_filt;

ReadInputHeight:
    for (unsigned i_ih = 0; i_ih < CONFIG_T::in_height; i_ih++) {
    ReadInputWidth:
        for (unsigned i_iw = 0; i_iw < CONFIG_T::in_width / (pack_factor); i_iw++) {
            #pragma HLS LOOP_FLATTEN
            if (res_T::size / CONFIG_T::n_filt == 1) {
                #pragma HLS PIPELINE II=pack_factor
            }
            compute_pool_encoded_2d<data_T, res_T, CONFIG_T>(i_ih, i_iw, data.read(), data_window, res, res_pack,
                                                             outputs_ready);
        }
    }
}

// Content of called function
void compute_pool_encoded_2d(
    const unsigned h_idx, const unsigned w_idx, const data_T &in_elem,
    hls::stream<typename data_T::value_type> data_window[CONFIG_T::pool_height * CONFIG_T::pool_width * CONFIG_T::n_filt],
    hls::stream<res_T> &res, res_T &res_pack, unsigned &outputs_ready) {
    // Nearest H without unused pixels on the right
    constexpr unsigned nH =
        ((CONFIG_T::in_height - CONFIG_T::pool_height) / CONFIG_T::stride_height) * CONFIG_T::stride_height +
        CONFIG_T::pool_height;
    // Scaled H that behaves like original H
    constexpr unsigned sH =
        (DIV_ROUNDUP(CONFIG_T::pool_height, CONFIG_T::stride_height) - 1) * CONFIG_T::stride_height + CONFIG_T::pool_height;
    // Nearest W without unused pixels on the right
    constexpr unsigned nW = ((CONFIG_T::in_width - CONFIG_T::pool_width) / CONFIG_T::stride_width) * CONFIG_T::stride_width +
                            CONFIG_T::pool_width;
    // Scaled W that behaves like original W
    constexpr unsigned sW =
        (DIV_ROUNDUP(CONFIG_T::pool_width, CONFIG_T::stride_width) - 1) * CONFIG_T::stride_width + CONFIG_T::pool_width;

#ifdef __SYNTHESIS__
    bool initialized = false;
    unsigned pool_table_height[CONFIG_T::in_height];
    unsigned pool_table_width[CONFIG_T::in_width];
#else
    static bool initialized = false;
    static unsigned pool_table_height[CONFIG_T::in_height];
    static unsigned pool_table_width[CONFIG_T::in_width];
#endif
    if (!initialized) {
        init_pool_table<CONFIG_T::in_height, CONFIG_T::pool_height>(pool_table_height);
        init_pool_table<CONFIG_T::in_width, CONFIG_T::pool_width>(pool_table_width);
        initialized = true;
    }

    #pragma HLS INLINE

    if (data_T::size / CONFIG_T::n_filt > 1) {
        #pragma HLS ARRAY_PARTITION variable=pool_table_height complete
        #pragma HLS ARRAY_PARTITION variable=pool_table_width complete
    }

    typename CONFIG_T::accum_t pool_window[CONFIG_T::pool_height * CONFIG_T::pool_width];
    #pragma HLS ARRAY_PARTITION variable=pool_window complete

    const unsigned sh_idx = pool_table_height[h_idx] * CONFIG_T::pool_width;
    const unsigned wp_idx = w_idx * (data_T::size / CONFIG_T::n_filt);

PixelLoop:
    for (unsigned p = 0; p < data_T::size / CONFIG_T::n_filt; p++) {
        #pragma HLS PIPELINE

        ap_uint<CONFIG_T::pool_height *CONFIG_T::pool_width> filt_mask = 0;
        if ((h_idx < nH) && (wp_idx + p < nW)) {
            filt_mask = sh_idx + pool_table_width[wp_idx + p] + 1;
        }

    CopyDataFilt:
        for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
            if (filt_mask > 0)
                data_window[c * CONFIG_T::pool_height * CONFIG_T::pool_width + filt_mask.to_uint() - 1].write(
                    in_elem[p * CONFIG_T::n_filt + c]);
        }

        if (filt_mask == CONFIG_T::pool_height * CONFIG_T::pool_width) {
        FiltLoop:
            for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
            PoolLoop:
                for (unsigned f = 0; f < CONFIG_T::pool_height * CONFIG_T::pool_width; f++) {
                    pool_window[f] = data_window[c * CONFIG_T::pool_height * CONFIG_T::pool_width + f].read();
                }
                if (res_T::size / CONFIG_T::n_filt ==
                    1) { // Saves resources if we don't pack output, compiler will remove the else branch
                    res_pack[c] =
                        reduce_pool<typename CONFIG_T::accum_t, CONFIG_T::pool_height * CONFIG_T::pool_width, CONFIG_T>(
                            pool_window);
                } else {
                    res_pack[outputs_ready * CONFIG_T::n_filt + c] =
                        reduce_pool<typename CONFIG_T::accum_t, CONFIG_T::pool_height * CONFIG_T::pool_width, CONFIG_T>(
                            pool_window);
                }
            }
            if (res_T::size / CONFIG_T::n_filt ==
                1) { // Saves resources if we don't pack output, compiler will remove the else branch
                res.write(res_pack);
            } else {
                if (outputs_ready == (res_T::size / CONFIG_T::n_filt) - 1) {
                    res.write(res_pack);
                    outputs_ready = 0;
                } else {
                    outputs_ready++;
                }
            }
        }
    }
}