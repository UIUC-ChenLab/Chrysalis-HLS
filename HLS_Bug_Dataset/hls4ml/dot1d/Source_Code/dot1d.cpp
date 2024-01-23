void dot1d(input1_T data1[CONFIG_T::n_in], input2_T data2[CONFIG_T::n_in], res_T res[CONFIG_T::n_out]) {
    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    #pragma HLS ALLOCATION operation instances=mul limit=CONFIG_T::multiplier_limit

    typename CONFIG_T::accum_t mult[CONFIG_T::n_in];
    #pragma HLS ARRAY_PARTITION variable=mult complete
    typename CONFIG_T::accum_t acc = 0;

Product:
    for (int i_mult = 0; i_mult < CONFIG_T::n_in; i_mult++) {
        #pragma HLS UNROLL
        mult[i_mult] = CONFIG_T::template product<input1_T, input2_T>::product(data1[i_mult], data2[i_mult]);
    }

Accum:
    for (int i_acc = 0; i_acc < CONFIG_T::n_in; i_acc++) {
        #pragma HLS UNROLL
        acc += mult[i_acc];
    }

    res[0] = cast<input1_T, res_T, CONFIG_T>(acc);
}