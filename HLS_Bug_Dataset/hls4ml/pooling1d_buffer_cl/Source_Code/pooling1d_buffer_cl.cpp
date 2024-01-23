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