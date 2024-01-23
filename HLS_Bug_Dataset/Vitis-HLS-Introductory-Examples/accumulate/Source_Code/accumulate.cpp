#define SZ 8

int accumulate(data_t A[]) {
#pragma HLS inline off

    data_t acc = 0.0;
    for (int i = 0; i < SZ; i++) {
        std::cout << "A: " << A[i] << std::endl;
        acc += A[i];
    }
    return acc;
}