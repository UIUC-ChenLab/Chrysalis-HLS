void softmax_argmax(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    for (int i = 0; i < CONFIG_T::n_in / res_T::size; i++) {
        #pragma HLS PIPELINE
        data_T in_data = data.read();
        res_T out_data;

        for (int i = 0; i < res_T::size; i++) {
            #pragma HLS UNROLL
            out_data[i] = (typename res_T::value_type)0;
        }

        typename data_T::value_type maximum = in_data[0];
        int idx = 0;

        for (int i = 1; i < res_T::size; i++) {
            #pragma HLS PIPELINE
            if (in_data[i] > maximum) {
                maximum = in_data[i];
                idx = i;
            }
        }

        out_data[idx] = (typename res_T::value_type)1;
        res.write(out_data);
    }
}