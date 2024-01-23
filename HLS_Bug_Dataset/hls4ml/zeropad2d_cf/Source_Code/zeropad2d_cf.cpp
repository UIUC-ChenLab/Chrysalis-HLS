void zeropad2d_cf(data_T data[CONFIG_T::n_chan * CONFIG_T::in_height * CONFIG_T::in_width],
                  data_T res[CONFIG_T::n_chan * CONFIG_T::out_height * CONFIG_T::out_width]) {
    #pragma HLS PIPELINE

    for (int k = 0; k < CONFIG_T::n_chan; k++) {

        for (int i = 0; i < CONFIG_T::pad_top; i++) {
            for (int j = 0; j < CONFIG_T::out_width; j++) {
                *(res++) = 0;
            }
        }

        for (int i = 0; i < CONFIG_T::in_height; i++) {
            for (int j = 0; j < CONFIG_T::pad_left; j++) {
                *(res++) = 0;
            }
            for (int j = 0; j < CONFIG_T::in_width; j++) {
                *(res++) = (res_T) * (data++);
            }
            for (int j = 0; j < CONFIG_T::pad_right; j++) {
                *(res++) = 0;
            }
        }

        for (int i = 0; i < CONFIG_T::pad_bottom; i++) {
            for (int j = 0; j < CONFIG_T::out_width; j++) {
                *(res++) = 0;
            }
        }
    }
}