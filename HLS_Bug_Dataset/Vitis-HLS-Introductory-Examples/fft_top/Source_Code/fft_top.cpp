void fft_top(bool direction, complex<data_in_t> in[FFT_LENGTH],
             complex<data_out_t> out[FFT_LENGTH], bool* ovflo) {
#pragma HLS interface ap_hs port = direction
#pragma HLS interface ap_fifo depth = 1 port = ovflo
#pragma HLS interface ap_fifo depth = FFT_LENGTH port = in, out
#pragma HLS dataflow

    complex<data_in_t> xn[FFT_LENGTH];
    complex<data_out_t> xk[FFT_LENGTH];
    config_t fft_config;
    status_t fft_status;

    inputdatamover(direction, &fft_config, in, xn);

    // FFT IP
    myfftwrapper(xn, xk, fft_status, fft_config);

    outputdatamover(&fft_status, ovflo, xk, out);
}

// Content of called function
void inputdatamover(bool direction, config_t* config, cmpxDataIn in[FFT_LENGTH],
                    cmpxDataIn out[FFT_LENGTH]) {
    config->setDir(direction);
    config->setSch(0x2AB);
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
}

// Content of called function
void myfftwrapper(cmpxDataIn xn[FFT_LENGTH], cmpxDataIn xk[FFT_LENGTH],
                  status_t& fft_status, config_t& fft_config) {
#pragma HLS dataflow
#pragma HLS INLINE recursive
    hls::fft<config1>(xn, xk, &fft_status, &fft_config);
}

// Content of called function
void outputdatamover(status_t* status_in, bool* ovflo,
                     cmpxDataOut in[FFT_LENGTH], cmpxDataOut out[FFT_LENGTH]) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
    *ovflo = status_in->getOvflo() & 0x1;
}