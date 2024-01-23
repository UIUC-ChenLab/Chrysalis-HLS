void broadcast_stream(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    if (CONFIG_T::in_height == 1 && CONFIG_T::in_width == 1 && CONFIG_T::in_chan == CONFIG_T::out_chan) {
        broadcast_stream_1x1xC<data_T, res_T, CONFIG_T>(data, res);
    } else if (CONFIG_T::in_chan == 1 && CONFIG_T::in_height == CONFIG_T::out_height &&
               CONFIG_T::in_width == CONFIG_T::out_width) {
        broadcast_stream_HxWx1<data_T, res_T, CONFIG_T>(data, res);
    }
}

// Content of called function
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

// Content of called function
void broadcast_stream_1x1xC(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    assert(CONFIG_T::in_height == 1 && CONFIG_T::in_width == 1 && CONFIG_T::in_chan == CONFIG_T::out_chan);
    int n_dupl = (CONFIG_T::out_height * CONFIG_T::out_width * CONFIG_T::out_chan) /
                 (CONFIG_T::in_height * CONFIG_T::in_width * CONFIG_T::in_chan);
BroadcastLoop:
    for (int i = 0; i < CONFIG_T::in_height * CONFIG_T::in_width * CONFIG_T::in_chan / data_T::size; i++) {
        #pragma HLS PIPELINE
        data_T in_data = data.read();
        for (int j = 0; j < n_dupl; j++) {
            #pragma HLS PIPELINE
            res_T out_data;
            PRAGMA_DATA_PACK(out_data)
            for (int k = 0; k < res_T::size; k++) {
                #pragma HLS UNROLL
                out_data[k] = in_data[k];
            }
            res.write(out_data);
        }
    }
}