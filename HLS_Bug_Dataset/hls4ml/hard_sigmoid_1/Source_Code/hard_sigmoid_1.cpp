void hard_sigmoid(hls::stream<data_T> &data, hls::stream<res_T> &res) {

HardSigmoidActLoop:
    for (int i = 0; i < CONFIG_T::n_in / res_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    HardSigmoidPackLoop:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            auto datareg = CONFIG_T::slope * in_data[j] + CONFIG_T::shift;
            if (datareg > 1)
                datareg = 1;
            else if (datareg < 0)
                datareg = 0;
            out_data[j] = datareg;
        }

        res.write(out_data);
    }
}