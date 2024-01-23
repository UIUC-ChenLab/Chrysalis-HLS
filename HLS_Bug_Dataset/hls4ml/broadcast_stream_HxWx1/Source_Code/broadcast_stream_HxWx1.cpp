void broadcast_stream_HxWx1(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::in_chan == 1 && CONFIG_T::in_height == CONFIG_T::out_height &&
           CONFIG_T::in_width == CONFIG_T::out_width);
BroadcastLoop:
    for (int i = 0; i < CONFIG_T::in_height * CONFIG_T::in_width * CONFIG_T::in_chan / data_T::size; i++) {
        #pragma HLS PIPELINE
        data_T in_data = data.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)
        for (int k = 0; k < res_T::size; k++) {
            #pragma HLS UNROLL
            out_data[k] = in_data[0];
        }
        res.write(out_data);
    }
}