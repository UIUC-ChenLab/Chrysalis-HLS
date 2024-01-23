void kernel_shift_2d(
    typename data_T::value_type shift_buffer[CONFIG_T::filt_height][CONFIG_T::n_chan],
    typename data_T::value_type kernel_window[CONFIG_T::filt_width * CONFIG_T::filt_height * CONFIG_T::n_chan]) {
    #pragma HLS inline

    // Shift kernel_window by one step to the left (manual shift operation)
    static const int filt_width = CONFIG_T::filt_width - 1;
KernelShiftWidth:
    for (int i_iw = 0; i_iw < filt_width; i_iw++) {
        #pragma HLS PIPELINE II = 1
    KernelShiftHeight:
        for (unsigned i_ih = 0; i_ih < CONFIG_T::filt_height; i_ih++) {
        KernelShiftChannel:
            for (unsigned i_ic = 0; i_ic < CONFIG_T::n_chan; i_ic++) {
                // Shift every element in kernel_window to the left
                kernel_window[i_ih * CONFIG_T::filt_width * CONFIG_T::n_chan + i_iw * CONFIG_T::n_chan + i_ic] =
                    kernel_window[i_ih * CONFIG_T::filt_width * CONFIG_T::n_chan + (i_iw + 1) * CONFIG_T::n_chan + i_ic];
            }
        }
    }

    // Insert shift_buffer column into right-most column of kernel
    static const int lastheight = (CONFIG_T::filt_width - 1) * CONFIG_T::n_chan;
KernelPushHeight:
    for (int i_ih = 0; i_ih < CONFIG_T::filt_height; i_ih++) {
        #pragma HLS UNROLL
    KernelPushChannel:
        for (int i_ic = 0; i_ic < CONFIG_T::n_chan; i_ic++) {
            kernel_window[lastheight + i_ih * CONFIG_T::filt_width * CONFIG_T::n_chan + i_ic] = shift_buffer[i_ih][i_ic];
        }
    }
}