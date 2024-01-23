void inputdatamover(bool direction, config_t* config, cmpxDataIn in[FFT_LENGTH],
                    cmpxDataIn out[FFT_LENGTH]) {
    config->setDir(direction);
    config->setSch(0x2AB);
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
}