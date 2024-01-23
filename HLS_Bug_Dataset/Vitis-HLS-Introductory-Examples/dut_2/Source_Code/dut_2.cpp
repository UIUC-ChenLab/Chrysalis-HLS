#define N 10

long dut(struct A& d) {
    long sum = 0;
    while (!d.s_in.empty())
        sum += d.s_in.read();
    for (unsigned i = 0; i < N; i++)
        sum += d.arr[i];
    return sum;
}