void leaky_relu(hls::stream<data_T> &data, typename data_T::value_type alpha, hls::stream<res_T> &res) {
LeakyReLUActLoop:
    for (int i = 0; i < CONFIG_T::n_in / res_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    LeakyReLUPackLoop:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            if (in_data[j] > 0)
                out_data[j] = in_data[j];
            else
                out_data[j] = alpha * in_data[j];
        }
        res.write(out_data);
    }
}