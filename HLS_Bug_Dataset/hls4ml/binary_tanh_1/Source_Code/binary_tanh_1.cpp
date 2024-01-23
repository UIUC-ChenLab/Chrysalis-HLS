void binary_tanh(hls::stream<data_T> &data, hls::stream<res_T> &res) {
PReLUActLoop:
    for (int i = 0; i < CONFIG_T::n_in / res_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    PReLUPackLoop:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            if (in_data[j] > 0)
                out_data[j] = (typename res_T::value_type)1;
            else
                out_data[j] = (typename res_T::value_type) - 1;
        }
        res.write(out_data);
    }
}