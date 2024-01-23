void concatenate3d(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2],
                   input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2],
                   res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 +
                             CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2]) {
    #pragma HLS INLINE

    if (CONFIG_T::axis == 3 || CONFIG_T::axis == -1) {
        concatenate3d_2<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    } else if (CONFIG_T::axis == 2 || CONFIG_T::axis == -2) {
        concatenate3d_1<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    } else {
        concatenate3d_0<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    }
}

// Content of called function
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

// Content of called function
void concatenate3d_1(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2],
                     input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2],
                     res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 +
                               CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0; ii++) {
        for (int jj = 0; jj < CONFIG_T::n_elem1_1; jj++) {
            for (int kk = 0; kk < CONFIG_T::n_elem1_2; kk++) {
                int res_idx =
                    ii * (CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_1) * CONFIG_T::n_elem1_2 + jj * CONFIG_T::n_elem1_2 + kk;
                int data_idx = ii * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 + jj * CONFIG_T::n_elem1_2 + kk;
                res[res_idx] = data1[data_idx];
            }
        }
        for (int jj = 0; jj < CONFIG_T::n_elem2_1; jj++) {
            for (int kk = 0; kk < CONFIG_T::n_elem2_2; kk++) {
                int res_idx = ii * (CONFIG_T::n_elem1_1 + CONFIG_T::n_elem2_1) * CONFIG_T::n_elem1_2 +
                              (jj + CONFIG_T::n_elem1_1) * CONFIG_T::n_elem1_2 + kk;
                int data_idx = ii * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2 + jj * CONFIG_T::n_elem2_2 + kk;
                res[res_idx] = data2[data_idx];
            }
        }
    }
}

// Content of called function
void concatenate3d_0(input1_T data1[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2],
                     input2_T data2[CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2],
                     res_T res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 +
                               CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2; ii++) {
        res[ii] = data1[ii];
    }
    for (int ii = 0; ii < CONFIG_T::n_elem2_0 * CONFIG_T::n_elem2_1 * CONFIG_T::n_elem2_2; ii++) {
        res[CONFIG_T::n_elem1_0 * CONFIG_T::n_elem1_1 * CONFIG_T::n_elem1_2 + ii] = data2[ii];
    }
}