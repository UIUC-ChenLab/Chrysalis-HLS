void example(volatile int* a) {

#pragma HLS INTERFACE m_axi port = a depth = 50

    int i;
    int buff[50];

    // memcpy creates a burst access to memory
    // multiple calls of memcpy cannot be pipelined and will be scheduled
    // sequentially memcpy requires a local buffer to store the results of the
    // memory transaction
    memcpy(buff, (const int*)a, 50 * sizeof(int));

    for (i = 0; i < 50; i++) {
        buff[i] = buff[i] + 100;
    }

    memcpy((int*)a, buff, 50 * sizeof(int));
}