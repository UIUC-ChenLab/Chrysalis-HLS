void softmax_legacy(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_in]) {
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

    #pragma HLS PIPELINE

    // Index into the lookup table based on data for exponentials
    typename CONFIG_T::table_t exp_res[CONFIG_T::n_in]; // different, independent, fixed point precision
    typename CONFIG_T::table_t exp_diff_res;            // different, independent, fixed point precision
    data_T data_cache[CONFIG_T::n_in];
    int data_round;
    int index;
    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
        data_cache[ii] = data[ii];
        exp_res[ii] = 0;
    }

    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
        for (int jj = 0; jj < CONFIG_T::n_in; jj++) {
            if (ii == jj)
                exp_diff_res = 1;
            else {
                data_round = (data_cache[jj] - data_cache[ii]) * CONFIG_T::table_size / 16;
                index = data_round + 8 * CONFIG_T::table_size / 16;
                if (index < 0)
                    index = 0;
                if (index > CONFIG_T::table_size - 1)
                    index = CONFIG_T::table_size - 1;
                exp_diff_res = exp_table[index];
            }
            exp_res[ii] += exp_diff_res;
        }
    }

    // Second loop to invert
    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
        int exp_res_index = exp_res[ii] * CONFIG_T::table_size / 64;
        if (exp_res_index < 0)
            exp_res_index = 0;
        if (exp_res_index > CONFIG_T::table_size - 1)
            exp_res_index = CONFIG_T::table_size - 1;
        // typename CONFIG_T::table_t exp_res_invert = invert_table[exp_res_index];
        res[ii] = (res_T)invert_table[exp_res_index];
    }
}