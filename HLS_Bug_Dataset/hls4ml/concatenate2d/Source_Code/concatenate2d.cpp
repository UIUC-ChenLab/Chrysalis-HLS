void concatenate2d(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1],
                   input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1],
                   res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1]) {
    #pragma HLS INLINE

    if (CONFIG_T::axis == 2 || CONFIG_T::axis == -1) {
        concatenate2d_1<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    } else {
        concatenate2d_0<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    }
}

// Content of called function
void concatenate2d_1(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1],
                     input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1],
                     res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0; ii++) {
        for (int jj = 0; jj < CONFIG_T::n_elem1_1; jj++) {
            res[ii * (CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_1) + jj] = data1[ii * CONFIG_T::n_elem1_1 + jj];
        }
        for (int jj = 0; jj < CONFIG_T::n_elem2_1; jj++) {
            res[ii * (CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_1) + CONFIG_T::n_elem1_1 + jj] =
                data2[ii * CONFIG_T::n_elem2_1 + jj];
        }
    }
}

// Content of called function
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