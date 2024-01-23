void compute_depthwise_output_buffer_1d(const data_T &in_elem, hls::stream<res_T> &res_stream,
                                        typename CONFIG_T::weight_t weights[CONFIG_T::kernel_size * CONFIG_T::n_chan],
                                        typename CONFIG_T::bias_t biases[CONFIG_T::n_chan]) {
    #pragma HLS INLINE

    // Thresholds
    const static int lShiftX = CONFIG_T::filt_width - 1;

    // Counters
    static int pX = 0;
    static int sX = 0;

    static typename data_T::value_type kernel_data[CONFIG_T::filt_width * CONFIG_T::n_chan];
    #pragma HLS ARRAY_PARTITION variable=kernel_data complete

    typename res_T::value_type res_out[CONFIG_T::n_chan];
    #pragma HLS ARRAY_PARTITION variable=res_out complete dim = 0

    res_T res_pack;
    PRAGMA_DATA_PACK(res_pack)

    // Add pixel to buffer
    nnet::kernel_shift_1d<data_T, CONFIG_T>(in_elem, kernel_data);

    // Check to see if we have a full kernel
    if ((sX - lShiftX) == 0 && pX > lShiftX - 1) {
        // Dense multiply
        #pragma HLS INLINE recursive
        if (CONFIG_T::strategy == nnet::latency) {
            depthwise_product<typename data_T::value_type, typename res_T::value_type, CONFIG_T>(kernel_data, res_out,
                                                                                                 weights, biases);
        } else {
            assert("Resource strategy for DepthwiseConv1D is not supported." && false);
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

    // Pointer Housekeeping
    if (pX + 1 == CONFIG_T::in_width) // Includes padding, end of line (padded)
    {
        pX = 0;
        sX = 0;
    } else {
        pX = pX + 1;
        sX = ((sX - lShiftX) == 0) ? sX - CONFIG_T::stride_width + 1 : sX + 1;
    }
}

// Content of called function
void multiply(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    assert(input1_T::size == input2_T::size && input1_T::size == res_T::size);

MultiplyLoop:
    for (int i = 0; i < CONFIG_T::n_elem / input1_T::size; i++) {
        #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

        input1_T in_data1 = data1.read();
        input2_T in_data2 = data2.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    MultiplyPack:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j] = in_data1[j] * in_data2[j];
        }

        res.write(out_data);
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
void depthwise_product(data_T data[CONFIG_T::kernel_size * CONFIG_T::n_chan], res_T res[CONFIG_T::n_chan],
                       typename CONFIG_T::weight_t weights[CONFIG_T::kernel_size * CONFIG_T::n_chan],
                       typename CONFIG_T::bias_t biases[CONFIG_T::n_chan]) {
    #pragma HLS INLINE

    typename CONFIG_T::accum_t mult[CONFIG_T::kernel_size * CONFIG_T::n_chan];
    typename CONFIG_T::accum_t acc[CONFIG_T::n_chan];

    // Use a function_instantiate in case it helps to explicitly optimize unchanging weights/biases
    #pragma HLS function_instantiate variable=weights

    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    #pragma HLS ARRAY_PARTITION variable=mult complete

    #pragma HLS ALLOCATION operation instances=mul limit=CONFIG_T::multiplier_limit

// Do the matrix-multiply
Product:
    for (int ii = 0; ii < CONFIG_T::kernel_size * CONFIG_T::n_chan; ii++) {
        #pragma HLS UNROLL
        mult[ii] = CONFIG_T::mult_config::template product<data_T, typename CONFIG_T::mult_config::weight_t>::product(
            data[ii], weights[ii]);
    }

// Initialize accumulator with input biases
ResetAccum:
    for (int iacc = 0; iacc < CONFIG_T::n_chan; iacc++) {
        #pragma HLS UNROLL
        acc[iacc] = (typename CONFIG_T::accum_t)biases[iacc];
    }

// Accumulate multiplication result
Accum1:
    for (int ii = 0; ii < CONFIG_T::kernel_size; ii++) {
    Accum2:
        for (int jj = 0; jj < CONFIG_T::n_chan; jj++) {
            int index = ii * CONFIG_T::n_chan + jj;
            acc[jj] += mult[index];
        }
    }

// Cast to "res_t" type
Result:
    for (int ires = 0; ires < CONFIG_T::n_chan; ires++) {
        #pragma HLS UNROLL
        res[ires] = cast<data_T, res_T, typename CONFIG_T::mult_config>(acc[ires]);
    }
}

// Content of called function
cast(typename CONFIG_T::accum_t x) {
    return (res_T)x;
}