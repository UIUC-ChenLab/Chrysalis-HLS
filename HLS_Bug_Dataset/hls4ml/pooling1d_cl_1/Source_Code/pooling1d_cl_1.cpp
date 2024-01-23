void pooling1d_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    #pragma HLS inline recursive
    switch (CONFIG_T::implementation) {
    case conv_implementation::linebuffer:
        pooling1d_buffer_cl<data_T, res_T, CONFIG_T>(data, res);
        break;
    case conv_implementation::encoded:
        pooling1d_encoded_cl<data_T, res_T, CONFIG_T>(data, res);
        break;
    }
}

// Content of called function
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

// Content of called function
void pooling1d_buffer_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0);

ReadInputWidth:
    for (unsigned i_iw = 0; i_iw < CONFIG_T::n_in; i_iw++) {
        #pragma HLS LOOP_FLATTEN
        #pragma HLS PIPELINE
        compute_pool_buffer_1d<data_T, res_T, CONFIG_T>(data.read(), res);
    }
}

// Content of called function
void compute_pool_buffer_1d(const data_T &in_elem, hls::stream<res_T> &res) {
    #pragma HLS INLINE
    const static int lShiftX = CONFIG_T::pool_width - 1;
    // Counters
    static int pX = 0;
    static int sX = 0;

    typename CONFIG_T::accum_t pool_window[CONFIG_T::pool_width];
    #pragma HLS ARRAY_PARTITION variable=pool_window complete

    static typename data_T::value_type kernel_data[CONFIG_T::pool_width * CONFIG_T::n_filt];
    #pragma HLS ARRAY_PARTITION variable = kernel_data complete dim = 0

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)

    // Add pixel into line buffer, return pooling kernels
    // 1D case line buffer not necessary. Put directly into the kernel_data buffer
    nnet::kernel_shift_1d<data_T, CONFIG_T>(in_elem, kernel_data);

    // Can compute pooling output
    if ((sX - lShiftX) == 0 && pX > lShiftX - 1) {
    FiltLoop:
        for (unsigned i_ic = 0; i_ic < CONFIG_T::n_filt; i_ic++) {
        #pragma HLS PIPELINE

        // Retrieve data for current channel
        PoolLoop:
            for (unsigned i_iw = 0; i_iw < CONFIG_T::pool_width; i_iw++) {
                pool_window[i_iw] = kernel_data[i_iw * CONFIG_T::n_filt + i_ic];
            }

            // Compute Pooling
            res_pack[i_ic] = reduce_pool<typename CONFIG_T::accum_t, CONFIG_T::pool_width, CONFIG_T>(pool_window);
        }

        // Write to output
        res.write(res_pack);
    }

    // Counter Housekeeping
    if (pX + 1 == CONFIG_T::n_in) // Includes padding, end of line (padded)
    {
        pX = 0;
        sX = 0;
    } else {
        pX = pX + 1;
        // Update stride (threshold) ? subtract stride : increment stride
        sX = ((sX - lShiftX) == 0) ? sX - CONFIG_T::stride_width + 1 : sX + 1;
    }
}

// Content of called function
void kernel_shift_1d(const data_T &in_elem,
                     typename data_T::value_type kernel_window[CONFIG_T::filt_width * CONFIG_T::n_chan]) {
    #pragma HLS inline

    // Shift kernel_window by one step to the left (manual shift operation)
    static const int filt_width = CONFIG_T::filt_width - 1;
KernelShiftWidth:
    for (int i_iw = 0; i_iw < filt_width; i_iw++) {
        #pragma HLS PIPELINE II = 1
    KernelShiftChannel:
        for (unsigned i_ic = 0; i_ic < CONFIG_T::n_chan; i_ic++) {
            #pragma HLS UNROLL
            // Shift every element in kernel_window to the left
            kernel_window[i_iw * CONFIG_T::n_chan + i_ic] = kernel_window[(i_iw + 1) * CONFIG_T::n_chan + i_ic];
        }
    }

    // Insert shift_buffer column into right-most column of kernel
    static const int lastheight = (CONFIG_T::filt_width - 1) * CONFIG_T::n_chan;
KernelPushChannel:
    for (int i_ic = 0; i_ic < CONFIG_T::n_chan; i_ic++) {
        #pragma HLS UNROLL
        kernel_window[lastheight + i_ic] = in_elem[i_ic];
    }
}

// Content of called function
void subtract(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    assert(input1_T::size == input2_T::size && input1_T::size == res_T::size);

SubtractLoop:
    for (int i = 0; i < CONFIG_T::n_elem / input1_T::size; i++) {
        #pragma HLS PIPELINE

        input1_T in_data1 = data1.read();
        input2_T in_data2 = data2.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    SubtractPack:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j] = in_data1[j] - in_data2[j];
        }

        res.write(out_data);
    }
}