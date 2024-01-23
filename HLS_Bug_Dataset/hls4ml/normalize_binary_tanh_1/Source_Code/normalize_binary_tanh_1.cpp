void normalize_binary_tanh(hls::stream<data_T> &data, hls::stream<nnet::array<ap_uint<1>, CONFIG_T::n_scale_bias>> &res,
                           typename data_T::value_type threshold[CONFIG_T::n_scale_bias]) {
    #pragma HLS ARRAY_PARTITION variable=threshold complete

BinaryNormLoop:
    for (int i = 0; i < CONFIG_T::n_in / data_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        nnet::array<ap_uint<1>, CONFIG_T::n_scale_bias> out_data;
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
            out_data[j] = (in_data[j] >= threshold[norm_index]) ? 1 : 0;
        }

        res.write(out_data);
    }
}