void normalize_ternary_tanh(hls::stream<data_T> &data, hls::stream<nnet::array<ap_int<2>, CONFIG_T::n_scale_bias>> &res,
                            typename data_T::value_type threshold_hi[CONFIG_T::n_scale_bias],
                            typename data_T::value_type threshold_lo[CONFIG_T::n_scale_bias]) {
    #pragma HLS ARRAY_PARTITION variable=threshold_hi complete
    #pragma HLS ARRAY_PARTITION variable=threshold_lo complete

TernaryNormLoop:
    for (int i = 0; i < CONFIG_T::n_in / data_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        nnet::array<ap_int<2>, CONFIG_T::n_scale_bias> out_data;
        PRAGMA_DATA_PACK(out_data)

    BatchNormPack:
        for (int j = 0; j < data_T::size; j++) {
            #pragma HLS UNROLL

            int norm_index;
            if (CONFIG_T::n_filt == -1) {
                norm_index = i * data_T::size + j;
            } else {
                norm_index = j % CONFIG_T::n_filt;
            }

            if (in_data[j] > threshold_hi[norm_index]) {
                out_data[j] = 1;
            } else if (in_data[j] <= threshold_lo[norm_index]) {
                out_data[j] = -1;
            } else {
                out_data[j] = 0;
            }
        }

        res.write(out_data);
    }
}