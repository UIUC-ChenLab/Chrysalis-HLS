void transpose_2d(data_T data[CONFIG_T::height * CONFIG_T::width], res_T data_t[CONFIG_T::height * CONFIG_T::width]) {
    #pragma HLS PIPELINE

    for (int i = 0; i < CONFIG_T::height; i++) {
        for (int j = 0; j < CONFIG_T::width; j++) {
            data_t[j * CONFIG_T::height + i] = data[i * CONFIG_T::width + j];
        }
    }
}