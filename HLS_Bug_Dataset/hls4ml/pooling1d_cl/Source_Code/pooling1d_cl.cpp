void pooling1d_cl(data_T data[CONFIG_T::n_in * CONFIG_T::n_filt], res_T res[CONFIG_T::n_out * CONFIG_T::n_filt]) {
    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    // TODO partition the arrays according to the reuse factor
    const int limit = pool_op_limit_1d<CONFIG_T>();
    #pragma HLS ALLOCATION function instances=CONFIG_T::pool_op limit=limit
    // Add any necessary padding
    unsigned padded_width = CONFIG_T::n_in + CONFIG_T::pad_left + CONFIG_T::pad_right;
    if (CONFIG_T::pad_left == 0 && CONFIG_T::pad_right == 0) {
        padded_width -= padded_width - (padded_width / CONFIG_T::stride_width * CONFIG_T::stride_width);
    }

    for (int ff = 0; ff < CONFIG_T::n_filt; ff++) {
        // Loop over input image x in steps of stride
        for (int ii = 0; ii < padded_width; ii += CONFIG_T::stride_width) {
            data_T pool[CONFIG_T::pool_width];
            #pragma HLS ARRAY_PARTITION variable=pool complete dim=0
            // Keep track of number of pixels in image vs padding region
            unsigned img_overlap = 0;
            // Loop over pool window x
            for (int jj = 0; jj < CONFIG_T::stride_width; jj++) {
                if (ii + jj < CONFIG_T::pad_left || ii + jj >= (padded_width - CONFIG_T::pad_right)) {
                    // Add padding
                    pool[jj] = pad_val<data_T, CONFIG_T::pool_op>();
                    if (CONFIG_T::count_pad)
                        img_overlap++;
                } else {
                    pool[jj] = data[(ii + jj - CONFIG_T::pad_left) * CONFIG_T::n_filt + ff];
                    img_overlap++;
                }
            }
            // do the pooling
            // TODO in the case of average pooling, need to reduce width to area of pool window
            // not overlapping padding region
            res[(ii / CONFIG_T::stride_width) * CONFIG_T::n_filt + ff] =
                pool_op<data_T, CONFIG_T::pool_width, CONFIG_T::pool_op>(pool);
            // If the pool op is Average, the zero-padding needs to be removed from the results
            if (CONFIG_T::pool_op == Average) {
                data_T rescale = static_cast<data_T>(CONFIG_T::pool_width) / img_overlap;
                res[(ii / CONFIG_T::stride_width) * CONFIG_T::n_filt + ff] *= rescale;
            }
        }
    }
}

// Content of called function
void average(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    assert(input1_T::size == input2_T::size && input1_T::size == res_T::size);

AverageLoop:
    for (int i = 0; i < CONFIG_T::n_elem / input1_T::size; i++) {
        #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

        input1_T in_data1 = data1.read();
        input2_T in_data2 = data2.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    AveragePack:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j] = (in_data1[j] + in_data2[j]) / (typename res_T::value_type)2;
        }

        res.write(out_data);
    }
}