void global_pooling2d_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::pad_top == 0 && CONFIG_T::pad_bottom == 0 && CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);
    assert(CONFIG_T::pool_height == CONFIG_T::stride_height && CONFIG_T::pool_width == CONFIG_T::stride_width);

    typename CONFIG_T::accum_t data_window[CONFIG_T::n_filt];
    #pragma HLS ARRAY_PARTITION variable=data_window complete

    typename CONFIG_T::accum_t init = 0;
    if (CONFIG_T::pool_op == Max) {
        init = hls::numeric_limits<typename CONFIG_T::accum_t>::min();
    }

PoolInitLoop:
    for (unsigned i_init = 0; i_init < CONFIG_T::n_filt; i_init++) {
        #pragma HLS UNROLL
        data_window[i_init] = init;
    }

ReadInputHeight:
    for (unsigned i_ih = 0; i_ih < CONFIG_T::in_height; i_ih++) {
    ReadInputWidth:
        for (unsigned i_iw = 0; i_iw < CONFIG_T::in_width / (data_T::size / CONFIG_T::n_filt); i_iw++) {
            #pragma HLS LOOP_FLATTEN
            compute_global_pool<data_T, res_T, CONFIG_T>(data.read(), data_window);
        }
    }

    if (CONFIG_T::pool_op == Max) {
    MaxPoolRes:
        for (unsigned i_res = 0; i_res < CONFIG_T::n_filt / res_T::size; i_res++) {
            #pragma HLS PIPELINE

            res_T res_pack;
            PRAGMA_DATA_PACK(res_pack)
        MaxPoolPack:
            for (unsigned i_pack = 0; i_pack < res_T::size; i_pack++) {
                #pragma HLS UNROLL
                res_pack[i_pack] = data_window[i_pack];
            }
            res.write(res_pack);
        }
    } else {
    AvgPoolRes:
        for (unsigned i_res = 0; i_res < CONFIG_T::n_filt / res_T::size; i_res++) {
            #pragma HLS PIPELINE

            res_T res_pack;
            PRAGMA_DATA_PACK(res_pack)
        AvgPoolPack:
            for (unsigned i_pack = 0; i_pack < res_T::size; i_pack++) {
                #pragma HLS UNROLL
                res_pack[i_pack] = data_window[i_pack] / (CONFIG_T::in_height * CONFIG_T::in_width);
            }
            res.write(res_pack);
        }
    }
}

// Content of called function
void compute_global_pool(const data_T &in_elem, typename CONFIG_T::accum_t data_window[CONFIG_T::n_filt]) {
PoolFilt:
    for (unsigned c = 0; c < CONFIG_T::n_filt; c++) {
        #pragma HLS UNROLL

        typename CONFIG_T::accum_t data_pack[data_T::size / CONFIG_T::n_filt];
        #pragma HLS ARRAY_PARTITION variable=data_pack complete dim=0

    PixelLoop:
        for (unsigned p = 0; p < data_T::size / CONFIG_T::n_filt; p++) {
            #pragma HLS UNROLL
            data_pack[p] = in_elem[p * CONFIG_T::n_filt + c];
        }
        data_window[c] = reduce_global_pool<typename CONFIG_T::accum_t, data_T::size / CONFIG_T::n_filt, CONFIG_T>(
            data_window[c], data_pack);
    }
}