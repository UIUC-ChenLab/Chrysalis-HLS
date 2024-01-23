void maximum(input1_T data1[CONFIG_T::n_elem], input2_T data2[CONFIG_T::n_elem], res_T res[CONFIG_T::n_elem]) {
    #pragma HLS PIPELINE

    for (int ii = 0; ii < CONFIG_T::n_elem; ii++) {
        res[ii] = (data1[ii] > data2[ii]) ? data1[ii] : data2[ii];
    }
}