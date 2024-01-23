void softmax_stable(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_in]) {
    #pragma HLS pipeline
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
        init_exp_table<data_T, CONFIG_T>(exp_table);
        // Note we are inverting the exponentials, which have type exp_table_t
        init_invert_table<typename CONFIG_T::exp_table_t, CONFIG_T>(invert_table);
        initialized = true;
    }

    // Find the max and compute all delta(x_i, x_max)
    Op_max<data_T> op_max;
    data_T x_max = reduce<data_T, CONFIG_T::n_in, Op_max<data_T>>(data, op_max);

    // For the diffs, use the same type as the input but force rounding and saturation
    ap_fixed<data_T::width, data_T::iwidth, AP_RND, AP_SAT> d_xi_xmax[CONFIG_T::n_in];
    for (unsigned i = 0; i < CONFIG_T::n_in; i++) {
        #pragma HLS unroll
        d_xi_xmax[i] = data[i] - x_max;
    }

    // Calculate all the e^x's
    typename CONFIG_T::exp_table_t exp_res[CONFIG_T::n_in];
    #pragma HLS array_partition variable=exp_res complete
    typename CONFIG_T::exp_table_t exp_sum(0);
    for (unsigned i = 0; i < CONFIG_T::n_in; i++) {
        #pragma HLS unroll
        unsigned x = softmax_idx_from_real_val<data_T, CONFIG_T>(d_xi_xmax[i]);
        exp_res[i] = exp_table[x];
    }

    // Explicitly sum the results with an adder tree.
    // Rounding & Saturation mode, which improve accuracy, prevent Vivado from expression balancing
    Op_add<typename CONFIG_T::exp_table_t> op_add;
    exp_sum =
        reduce<typename CONFIG_T::exp_table_t, CONFIG_T::n_in, Op_add<typename CONFIG_T::exp_table_t>>(exp_res, op_add);

    typename CONFIG_T::inv_table_t inv_exp_sum =
        invert_table[softmax_idx_from_real_val<typename CONFIG_T::exp_table_t, CONFIG_T>(exp_sum)];
    for (unsigned i = 0; i < CONFIG_T::n_in; i++) {
        #pragma HLS unroll
        res[i] = exp_res[i] * inv_exp_sum;
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