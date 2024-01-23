void zeropad1d_cf(data_T data[CONFIG_T::n_chan * CONFIG_T::in_width], data_T res[CONFIG_T::n_chan * CONFIG_T::out_width]) {
    #pragma HLS PIPELINE

    for (int j = 0; j < CONFIG_T::n_chan; j++) {
        for (int i = 0; i < CONFIG_T::pad_left; i++) {
            *(res++) = 0;
        }

        for (int i = 0; i < CONFIG_T::in_width; i++) {
            *(res++) = (res_T) * (data++);
        }

        for (int i = 0; i < CONFIG_T::pad_right; i++) {
            *(res++) = 0;
        }
    }
}