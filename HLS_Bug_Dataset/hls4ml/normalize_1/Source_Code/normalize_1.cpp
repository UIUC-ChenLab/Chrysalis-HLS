void normalize(hls::stream<data_T> &data, hls::stream<res_T> &res, typename CONFIG_T::scale_t scale[CONFIG_T::n_scale_bias],
               typename CONFIG_T::bias_t bias[CONFIG_T::n_scale_bias]) {
    #pragma HLS ARRAY_PARTITION variable=scale complete
    #pragma HLS ARRAY_PARTITION variable=bias complete

    constexpr unsigned ii = CONFIG_T::n_in / CONFIG_T::multiplier_limit;
    #pragma HLS ALLOCATION operation instances=mul limit=CONFIG_T::multiplier_limit

BatchNormLoop:
    for (int i = 0; i < CONFIG_T::n_in / data_T::size; i++) {
        #pragma HLS PIPELINE II=ii

        data_T in_data = data.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    BatchNormpack:
        for (int j = 0; j < data_T::size; j++) {
            #pragma HLS UNROLL
            int norm_index;
            if (CONFIG_T::n_filt == -1) {
                norm_index = i * data_T::size + j;
            } else {
                norm_index = j % CONFIG_T::n_filt;
            }
            out_data[j] = CONFIG_T::template product<typename data_T::value_type, typename CONFIG_T::scale_t>::product(
                              in_data[j], scale[norm_index]) +
                          bias[norm_index];
        }

        res.write(out_data);
    }
}