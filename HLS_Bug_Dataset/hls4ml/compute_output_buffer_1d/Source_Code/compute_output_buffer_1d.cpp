void compute_output_buffer_1d(
    const data_T &in_elem, hls::stream<res_T> &res_stream,
    typename CONFIG_T::weight_t weights[CONFIG_T::kernel_size * CONFIG_T::n_chan * CONFIG_T::n_filt],
    typename CONFIG_T::bias_t biases[CONFIG_T::n_filt]) {
    #pragma HLS INLINE

    // Thresholds
    const static int lShiftX = CONFIG_T::filt_width - 1;

    // Counters
    static int pX = 0; // pixel counter
    static int sX = 0; // stride counter

    static typename data_T::value_type kernel_data[CONFIG_T::filt_width * CONFIG_T::n_chan];
    #pragma HLS ARRAY_PARTITION variable = kernel_data complete

    typename res_T::value_type res_out[CONFIG_T::n_filt];
    #pragma HLS ARRAY_PARTITION variable = res_out complete dim = 0

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)

    // Add pixel to buffer
    nnet::kernel_shift_1d<data_T, CONFIG_T>(in_elem, kernel_data);

    // Check to see if we have a full kernel
    if ((sX - lShiftX) == 0 && pX > lShiftX - 1) {

        // Dense multiply
        #pragma HLS INLINE recursive
        if (CONFIG_T::strategy == nnet::latency) {
            dense_latency<typename data_T::value_type, typename res_T::value_type, typename CONFIG_T::mult_config>(
                kernel_data, res_out, weights, biases);
        } else {
            dense_resource<typename data_T::value_type, typename res_T::value_type, typename CONFIG_T::mult_config>(
                kernel_data, res_out, weights, biases);
        }

    // Pack output
    CastLoop:
        for (unsigned i_ic = 0; i_ic < CONFIG_T::n_filt; i_ic++) {
            #pragma HLS UNROLL
            res_pack[i_ic] = res_out[i_ic];
        }

        // Write output to stream when output ready
        res_stream.write(res_pack);
    }

    // Counter Housekeeping
    if (pX + 1 == CONFIG_T::in_width) // Includes padding, end of line (padded)
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