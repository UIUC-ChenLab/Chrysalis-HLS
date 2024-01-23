void transpose_2d(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    typename data_T::value_type data_array[CONFIG_T::height * CONFIG_T::width];
    #pragma HLS ARRAY_PARTITION variable=data_array complete

    for (int i = 0; i < CONFIG_T::height * CONFIG_T::width / data_T::size; i++) {
        #pragma HLS PIPELINE
        data_T in_data = data.read();
        for (int j = 0; j < data_T::size; j++) {
            data_array[i * data_T::size + j] = typename data_T::value_type(in_data[j]);
        }
    }

    for (int i = 0; i < CONFIG_T::height * CONFIG_T::width / res_T::size; i++) {
        #pragma HLS PIPELINE
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)
        for (int j = 0; j < res_T::size; j++) {
            out_data[j] = typename res_T::value_type(data_array[j * data_T::size + i]);
        }
        res.write(out_data);
    }
}