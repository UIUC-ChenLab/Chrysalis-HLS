void softmax_argmax(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_in]) {
    for (int i = 0; i < CONFIG_T::n_in; i++) {
        #pragma HLS UNROLL
        res[i] = (res_T)0;
    }

    data_T maximum = data[0];
    int idx = 0;

    for (int i = 1; i < CONFIG_T::n_in; i++) {
        #pragma HLS PIPELINE
        if (data[i] > maximum) {
            maximum = data[i];
            idx = i;
        }
    }

    res[idx] = (res_T)1;
}