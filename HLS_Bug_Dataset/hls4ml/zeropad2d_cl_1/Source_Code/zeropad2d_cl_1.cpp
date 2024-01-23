void zeropad2d_cl(hls::stream<data_T> &data, hls::stream<res_T> &res) {

PadTop:
    for (int i = 0; i < CONFIG_T::pad_top; i++) {
    PadTopWidth:
        for (int j = 0; j < CONFIG_T::out_width; j++) {
            fill_zero<res_T, CONFIG_T>(res);
        }
    }

PadMain:
    for (int i = 0; i < CONFIG_T::in_height; i++) {
    PadLeft:
        for (int j = 0; j < CONFIG_T::pad_left; j++) {
            fill_zero<res_T, CONFIG_T>(res);
        }
    CopyMain:
        for (int j = 0; j < CONFIG_T::in_width; j++) {
            fill_data<data_T, res_T, CONFIG_T>(data, res);
        }
    PadRight:
        for (int j = 0; j < CONFIG_T::pad_right; j++) {
            fill_zero<res_T, CONFIG_T>(res);
        }
    }

PadBottom:
    for (int i = 0; i < CONFIG_T::pad_bottom; i++) {
    PadBottomWidth:
        for (int j = 0; j < CONFIG_T::out_width; j++) {
            fill_zero<res_T, CONFIG_T>(res);
        }
    }
}