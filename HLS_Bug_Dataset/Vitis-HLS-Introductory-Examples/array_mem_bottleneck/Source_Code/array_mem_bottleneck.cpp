dout_t array_mem_bottleneck(din_t mem[N]) {

    dout_t sum = 0;
    int i;

SUM_LOOP:
    for (i = 2; i < N; ++i)
        sum += mem[i] + mem[i - 1] + mem[i - 2];

    return sum;
}