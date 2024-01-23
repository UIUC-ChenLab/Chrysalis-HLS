void outputdatamover(status_t* status_in, bool* ovflo,
                     cmpxDataOut in[FFT_LENGTH], cmpxDataOut out[FFT_LENGTH]) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
    *ovflo = status_in->getOvflo() & 0x1;
}