void concatenate2d_0(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1],
                     input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1],
                     res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1; ii++) {
        res[ii] = data1[ii];
    }
    for (int ii = 0; ii < CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1; ii++) {
        res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 + ii] = data2[ii];
    }
}