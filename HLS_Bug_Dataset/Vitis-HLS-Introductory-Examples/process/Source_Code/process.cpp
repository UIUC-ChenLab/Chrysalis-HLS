#define N 10

void process(dout_t x_aux[N], dout_t y_aux[N], din_t factor) {

    for (int i = 0; i < N / 4; i++) {
        x_aux[i] = factor + x_aux[i];
        y_aux[i] = factor + y_aux[i];
    }

    for (int i = N - N / 4; i < N; i++) {
        x_aux[i] = x_aux[i] - factor;
        y_aux[i] = y_aux[i] - factor;
    }
}