void concatenate3d_2(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2],
                     input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2],
                     res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 +
                               CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0; ii++) {
        for (int jj = 0; jj < CONFIG_T::n_elem1_1; jj++) {
            for (int kk = 0; kk < CONFIG_T::n_elem1_2; kk++) {
                int res_idx = ii * CONFIG_T::n_elem1_1 * (CONFIG_T::n_elem1_2 + CONFIG_T::n_elem2_2) +
                              jj * (CONFIG_T::n_elem1_2 + CONFIG_T::n_elem2_2) + kk;
                int data_idx = ii * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 + jj * CONFIG_T::n_elem1_2 + kk;
                res[res_idx] = data1[data_idx];
            }
            for (int kk = 0; kk < CONFIG_T::n_elem1_2; kk++) {
                int res_idx = ii * CONFIG_T::n_elem1_1 * (CONFIG_T::n_elem1_2 + CONFIG_T::n_elem2_2) +
                              jj * (CONFIG_T::n_elem1_2 + CONFIG_T::n_elem2_2) + kk + CONFIG_T::n_elem1_2;
                int data_idx = ii * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2 + jj * CONFIG_T::n_elem2_2 + kk;
                res[res_idx] = data2[data_idx];
            }
        }
    }
}