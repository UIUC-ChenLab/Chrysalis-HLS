void softmax_latency(hls::stream<data_T> &data, hls::stream<res_T> &res) {
    // Initialize the lookup tables
#ifdef __HLS_SYN__
    bool initialized = false;
    typename CONFIG_T::exp_table_t exp_table[CONFIG_T::table_size];
    typename CONFIG_T::inv_table_t invert_table[CONFIG_T::table_size];
#else
    static bool initialized = false;
    static typename CONFIG_T::exp_table_t exp_table[CONFIG_T::table_size];
    static typename CONFIG_T::inv_table_t invert_table[CONFIG_T::table_size];

#endif
    if (!initialized) {
        // Note we are exponentiating the inputs, which have type data_T
        init_exp_table<typename data_T::value_type, CONFIG_T>(exp_table);
        // Note we are inverting the exponentials, which have type exp_table_t
        init_invert_table<typename CONFIG_T::exp_table_t, CONFIG_T>(invert_table);
        initialized = true;
    }

    constexpr unsigned multiplier_limit = DIV_ROUNDUP(data_T::size, CONFIG_T::reuse_factor);
    constexpr unsigned ii = data_T::size / multiplier_limit;

    // Calculate all the e^x's
    typename CONFIG_T::exp_table_t exp_res[data_T::size];
    #pragma HLS array_partition variable=exp_res complete
    typename CONFIG_T::exp_table_t exp_sum(0);
SoftmaxExpLoop:
    for (unsigned i = 0; i < CONFIG_T::n_in / data_T::size; i++) {
        #pragma HLS PIPELINE II=ii

        data_T in_pack = data.read();
    SoftmaxExpPackLoop:
        for (unsigned j = 0; j < data_T::size; j++) {
            #pragma HLS UNROLL
            unsigned x = softmax_idx_from_real_val<typename data_T::value_type, CONFIG_T>(in_pack[j]);
            exp_res[j] = exp_table[x];
        }

        // Explicitly sum the results with an adder tree.
        // Rounding & Saturation mode, which improve accuracy, prevent Vivado from expression balancing
        Op_add<typename CONFIG_T::exp_table_t> op_add;
        exp_sum =
            reduce<typename CONFIG_T::exp_table_t, data_T::size, Op_add<typename CONFIG_T::exp_table_t>>(exp_res, op_add);

        typename CONFIG_T::inv_table_t inv_exp_sum =
            invert_table[softmax_idx_from_real_val<typename CONFIG_T::exp_table_t, CONFIG_T>(exp_sum)];

        res_T out_pack;
        PRAGMA_DATA_PACK(out_pack)

    SoftmaxInvPackLoop:
        for (unsigned j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            #pragma HLS ALLOCATION operation instances=mul limit=multiplier_limit
            out_pack[j] = exp_res[j] * inv_exp_sum;
        }
        res.write(out_pack);
    }
}

// Content of called function
void init_exp_table(typename CONFIG_T::exp_table_t table_out[CONFIG_T::table_size]) {
    // The template data_T is the data type used to address the table
    for (unsigned i = 0; i < CONFIG_T::table_size; i++) {
        // Slicing bits for address is going to round towards 0, so take the central value
        float x = softmax_real_val_from_idx<data_T, CONFIG_T>(i);
        typename CONFIG_T::exp_table_t exp_x = exp_fcn_float(x);
        table_out[i] = exp_x;
    }
}

// Content of called function
void init_invert_table(typename CONFIG_T::inv_table_t table_out[CONFIG_T::table_size]) {
    // The template data_T is the data type used to address the table
    for (unsigned i = 0; i < CONFIG_T::table_size; i++) {
        float x = softmax_real_val_from_idx<data_T, CONFIG_T>(i);
        typename CONFIG_T::inv_table_t inv_x = 1 / x;
        table_out[i] = inv_x;
    }
}