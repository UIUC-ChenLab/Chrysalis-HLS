void softmax_legacy(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    // Initialize the lookup table
#ifdef __HLS_SYN__
    bool initialized = false;
    typename CONFIG_T::table_t exp_table[CONFIG_T::table_size];
    typename CONFIG_T::table_t invert_table[CONFIG_T::table_size];
#else
    static bool initialized = false;
    static typename CONFIG_T::table_t exp_table[CONFIG_T::table_size];
    static typename CONFIG_T::table_t invert_table[CONFIG_T::table_size];
#endif
    if (!initialized) {
        init_exp_table_legacy<CONFIG_T, CONFIG_T::table_size>(exp_table);
        init_invert_table_legacy<CONFIG_T, CONFIG_T::table_size>(invert_table);
        initialized = true;
    }

    // Index into the lookup table based on data for exponentials
    typename CONFIG_T::table_t exp_res[data_T::size];
    typename CONFIG_T::table_t exp_diff_res;
    typename data_T::value_type data_cache[data_T::size];

SoftmaxInitLoop:
    for (unsigned s = 0; s < CONFIG_T::n_in / data_T::size; s++) {
        #pragma HLS PIPELINE
        data_T in_pack = data.read();
    SoftmaxInitPackLoop:
        for (unsigned j = 0; j < data_T::size; j++) {
            #pragma HLS UNROLL
            data_cache[j] = in_pack[j];
            exp_res[j] = 0;
        }

    SoftmaxExpLoop:
        for (int i = 0; i < data_T::size; i++) {
        #pragma HLS UNROLL
        SoftmaxExpInner:
            for (int j = 0; j < data_T::size; j++) {
                #pragma HLS UNROLL

                if (i == j) {
                    exp_diff_res = 1;
                } else {
                    int data_round = (data_cache[j] - data_cache[i]) * CONFIG_T::table_size / 16;
                    int index = data_round + 8 * CONFIG_T::table_size / 16;
                    if (index < 0)
                        index = 0;
                    if (index > CONFIG_T::table_size - 1)
                        index = CONFIG_T::table_size - 1;
                    exp_diff_res = exp_table[index];
                }

                exp_res[i] += exp_diff_res;
            }
        }

        res_T out_pack;
        PRAGMA_DATA_PACK(out_pack)

    SoftmaxInvPackLoop:
        for (unsigned j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL

            int exp_res_index = exp_res[j] * CONFIG_T::table_size / 64;
            if (exp_res_index < 0)
                exp_res_index = 0;
            if (exp_res_index > CONFIG_T::table_size - 1)
                exp_res_index = CONFIG_T::table_size - 1;

            out_pack[j] = (typename res_T::value_type)invert_table[exp_res_index];
        }
        res.write(out_pack);
    }
}