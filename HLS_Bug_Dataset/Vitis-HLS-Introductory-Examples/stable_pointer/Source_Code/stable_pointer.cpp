void stable_pointer(int* mem, hls::stream<int>& in, hls::stream<int>& out) {
#pragma HLS DATAFLOW
#pragma HLS INTERFACE mode = m_axi bundle = gmem depth =                       \
    256 max_read_burst_length = 16 max_widen_bitwidth =                        \
        512 max_write_burst_length = 16 num_read_outstanding =                 \
            16 num_write_outstanding = 16 port = mem
#pragma HLS stable variable = mem

    hls_thread_local hls::stream<int> int_fifo("int_fifo");
#pragma HLS STREAM depth = 512 type = fifo variable = int_fifo
    hls_thread_local hls::stream<int> int_fifo2("int_fifo2");
#pragma HLS STREAM depth = 512 type = fifo variable = int_fifo2

    hls_thread_local hls::task t1(process_23, in, int_fifo);
    hls_thread_local hls::task t2(process_11, int_fifo, int_fifo2);
    hls_thread_local hls::task t3(write_process, int_fifo2, out, mem);
}

// Content of called function
void write_process(hls::stream<int>& in, hls::stream<int>& out, int* mem) {
    int val;
    static int addr = 0;

    in.read(val);
    if (addr >= 32)
        addr = 0;
    mem[addr] = val;
    addr++;
    val = mem[addr - 1];
    out.write(val);
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
void process_11(hls::stream<int>& in, hls::stream<int>& out) {
#pragma HLS INLINE off
    static int state = 0;
    static int val;

    in.read(val);
    val = val * 11;
    out.write(val);
}

// Content of called function
void process_23(hls::stream<int>& in, hls::stream<int>& out) {
#pragma HLS INLINE off
    static int state = 0;
    static int val;

    in.read(val);
    val = val * 23;
    out.write(val);
}