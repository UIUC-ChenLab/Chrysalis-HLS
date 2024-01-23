void concatenate1d(input1_T data1[CONFIG_T::n_elem1_0], input2_T data2[CONFIG_T::n_elem2_0],
                   res_T res[CONFIG_T::n_elem1_0 + CONFIG_T::n_elem2_0]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0; ii++) {
        res[ii] = data1[ii];
    }
    for (int ii = 0; ii < CONFIG_T::n_elem2_0; ii++) {
        res[CONFIG_T::n_elem1_0 + ii] = data2[ii];
    }
}