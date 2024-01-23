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
cast(typename CONFIG_T::accum_t x) {
    return (res_T)x;
}