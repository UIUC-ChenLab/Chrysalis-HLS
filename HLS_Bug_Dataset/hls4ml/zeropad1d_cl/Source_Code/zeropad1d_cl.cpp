void zeropad1d_cl(data_T data[CONFIG_T::n_chan * CONFIG_T::in_width], res_T res[CONFIG_T::n_chan * CONFIG_T::out_width]) {
    #pragma HLS PIPELINE

    for (int i = 0; i < CONFIG_T::pad_left; i++) {
        for (int j = 0; j < CONFIG_T::n_chan; j++) {
            *(res++) = 0;
        }
    }

    for (int i = 0; i < CONFIG_T::in_width; i++) {
        for (int j = 0; j < CONFIG_T::n_chan; j++) {
            *(res++) = (res_T) * (data++);
        }
    }

    for (int i = 0; i < CONFIG_T::pad_right; i++) {
        for (int j = 0; j < CONFIG_T::n_chan; j++) {
            *(res++) = 0;
        }
    }
}