void pooling1d_encoded_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::pool_width == CONFIG_T::stride_width);

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)
    unsigned outputs_ready = 0;

    hls::stream<typename data_T::value_type> data_window[CONFIG_T::pool_width * CONFIG_T::n_filt];
    constexpr int win_depth = CONFIG_T::n_out;
    for (unsigned i_out = 0; i_out < CONFIG_T::pool_width * CONFIG_T::n_filt; i_out++) {
        #pragma HLS STREAM variable=data_window[i_out] depth=win_depth
    }

    constexpr int pack_factor = data_T::size / CONFIG_T::n_filt;

ReadInputWidth:
    for (unsigned i_iw = 0; i_iw < CONFIG_T::n_in / (pack_factor); i_iw++) {
        #pragma HLS LOOP_FLATTEN
        if (res_T::size / CONFIG_T::n_filt == 1) {
            #pragma HLS PIPELINE II=pack_factor
        }
        compute_pool_encoded_1d<data_T, res_T, CONFIG_T>(i_iw, data.read(), data_window, res, res_pack, outputs_ready);
    }
}

// Content of called function
void compute_pool_encoded_1d(const unsigned w_idx, const data_T &in_elem,
                             hls::stream<typename data_T::value_type> data_window[CONFIG_T::pool_width * CONFIG_T::n_filt],
                             hls::stream<res_T> &res, res_T &res_pack, unsigned &outputs_ready) {
    // Nearest W without unused pixels on the right
    constexpr unsigned nW =
        ((CONFIG_T::n_in - CONFIG_T::pool_width) / CONFIG_T::stride_width) * CONFIG_T::stride_width + CONFIG_T::pool_width;
    // Scaled W that behaves like original W
    constexpr unsigned sW =
        (DIV_ROUNDUP(CONFIG_T::pool_width, CONFIG_T::stride_width) - 1) * CONFIG_T::stride_width + CONFIG_T::pool_width;

#ifdef __SYNTHESIS__
    bool initialized = false;
    unsigned pool_table_width[CONFIG_T::n_in];
#else
    static bool initialized = false;
    static unsigned pool_table_width[CONFIG_T::n_in];
#endif
    if (!initialized) {
        init_pool_table<CONFIG_T::n_in, CONFIG_T::pool_width>(pool_table_width);
        initialized = true;
    }

    #pragma HLS INLINE

    if (data_T::size / CONFIG_T::n_filt > 1) {
        #pragma HLS ARRAY_PARTITION variable=pool_table_width complete
    }

    typename CONFIG_T::accum_t pool_window[CONFIG_T::pool_width];
    #pragma HLS ARRAY_PARTITION variable=pool_window complete

    const unsigned wp_idx = w_idx * (data_T::size / CONFIG_T::n_filt);

PixelLoop:
    for (unsigned p = 0; p < data_T::size / CONFIG_T::n_filt; p++) {
        #pragma HLS PIPELINE

        ap_uint<CONFIG_T::pool_width> filt_mask = 0;
        if (wp_idx + p < nW) {
            filt_mask = pool_table_width[wp_idx + p] + 1;
        }

    CopyDataFilt:
        for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
            if (filt_mask > 0)
                data_window[c * CONFIG_T::pool_width + filt_mask.to_uint() - 1].write(in_elem[p * CONFIG_T::n_filt + c]);
        }

        if (filt_mask == CONFIG_T::pool_width) {
        FiltLoop:
            for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
            PoolLoop:
                for (unsigned f = 0; f < CONFIG_T::pool_width; f++) {
                    pool_window[f] = data_window[c * CONFIG_T::pool_width + f].read();
                }
                if (res_T::size / CONFIG_T::n_filt ==
                    1) { // Saves resources if we don't pack output, compiler will remove the else branch
                    res_pack[c] = reduce_pool<typename CONFIG_T::accum_t, CONFIG_T::pool_width, CONFIG_T>(pool_window);
                } else {
                    res_pack[outputs_ready * CONFIG_T::n_filt + c] =
                        reduce_pool<typename CONFIG_T::accum_t, CONFIG_T::pool_width, CONFIG_T>(pool_window);
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