void compute_scaled_indices_1d(const unsigned w_idx, ap_uint<CONFIG_T::filt_width> *pixel_idx) {
    unsigned wp_idx = w_idx * (data_T::size / CONFIG_T::n_chan);

ComputeIndex:
    for (unsigned p = 0; p < data_T::size / CONFIG_T::n_chan; p++) {
        #pragma HLS UNROLL
        unsigned sw_idx =
            CONFIG_T::template scale_index<CONFIG_T::filt_width, CONFIG_T::stride_width, CONFIG_T::in_width>::scale_index(
                wp_idx + p);
        pixel_idx[p] = CONFIG_T::pixels[sw_idx];
    }
}