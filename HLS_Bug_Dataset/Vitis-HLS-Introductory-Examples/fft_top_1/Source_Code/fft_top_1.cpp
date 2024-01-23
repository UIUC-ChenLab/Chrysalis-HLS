void fft_top(bool direction, hls::stream<cmpxDataIn>& in,
             hls::stream<cmpxDataOut>& out, bool* ovflo) {
#pragma HLS interface ap_hs port = direction
#pragma HLS interface ap_fifo depth = 1 port = ovflo
#pragma HLS interface ap_fifo depth = FFT_LENGTH port = in, out
#pragma HLS dataflow

    hls::stream<complex<data_in_t>> xn;
    hls::stream<complex<data_out_t>> xk;
    hls::stream<config_t> fft_config;
    hls::stream<status_t> fft_status;

    inputdatamover(direction, fft_config, in, xn);

    // FFT IP
    hls::fft<config1>(xn, xk, fft_status, fft_config);

    outputdatamover(fft_status, ovflo, xk, out);
}

// Content of called function
void inputdatamover(bool direction, hls::stream<config_t>& config_strm,
                    hls::stream<cmpxDataIn>& in,
                    hls::stream<cmpxDataIn>& out_strm) {
    config_t config;
    config.setDir(direction);
    config.setSch(0x2AB);
    config_strm.write(config);
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
#pragma HLS pipeline II = 1 rewind
        out_strm.write(in.read());
    }
}

// Content of called function
#define N 10

void write(hls::burst_maxi<dout_t> RES, dout_t x_aux[N], dout_t y_aux[N]) {
    RES.write_request(
        0, N / 4); // request to write N/4 elements from the first element
    RES.write_request(
        N - N / 4,
        N / 4); // request to write N/4 elements from the last quarter element

    for (int i = 0; i < N / 2; i++) {
        if (i < N / 4)
            RES.write(x_aux[i] - y_aux[i]);
        else
            RES.write(x_aux[N - N / 2 + i] - y_aux[N - N / 2 + i] / N);
    }
    RES.write_response(); // wait for the write operation to complete
    RES.write_response(); // wait for the write operation to complete
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}

// Content of called function
void outputdatamover(hls::stream<status_t>& status_in_strm, bool* ovflo,
                     hls::stream<cmpxDataOut>& in_strm,
                     hls::stream<cmpxDataOut>& out) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
#pragma HLS pipeline II = 1 rewind
        out.write(in_strm.read());
    }
    status_t status = status_in_strm.read();
    *ovflo = status.getOvflo() & 0x1;
}