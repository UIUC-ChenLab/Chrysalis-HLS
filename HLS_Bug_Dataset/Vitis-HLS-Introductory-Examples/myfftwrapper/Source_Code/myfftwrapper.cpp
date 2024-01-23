void myfftwrapper(cmpxDataIn xn[FFT_LENGTH], cmpxDataIn xk[FFT_LENGTH],
                  status_t& fft_status, config_t& fft_config) {
#pragma HLS dataflow
#pragma HLS INLINE recursive
    hls::fft<config1>(xn, xk, &fft_status, &fft_config);
}