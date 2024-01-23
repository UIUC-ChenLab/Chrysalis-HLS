#define BURSTBUFFERSIZE 256

void vadd(int* a, int size, int inc_value) {
// Map pointer a to AXI4-master interface for global memory access
#pragma HLS INTERFACE m_axi port = a offset = slave bundle = gmem max_read_burst_length = 256 max_write_burst_length = 256 depth = 2048
// We also need to map a and return to a bundled axilite slave interface
#pragma HLS INTERFACE s_axilite port = a
#pragma HLS INTERFACE s_axilite port = size
#pragma HLS INTERFACE s_axilite port = inc_value
#pragma HLS INTERFACE s_axilite port = return

    int burstbuffer[BURSTBUFFERSIZE];

read_buf:
    // Per iteration of this loop perform BURSTBUFFERSIZE vector addition
    for (int i = 0; i < size; i += BURSTBUFFERSIZE) {
#pragma HLS LOOP_TRIPCOUNT min = c_size_min* c_size_min max = c_chunk_sz * c_chunk_sz / (c_size_max * c_size_max)
        int chunk_size = BURSTBUFFERSIZE;
        // boundary checks
        if ((i + BURSTBUFFERSIZE) > size) chunk_size = size - i;
        // burst read
        // Auto-pipeline is going to apply pipeline to these loops
        for (int j = 0; j < chunk_size; j++) {
// As the outer loop is not a perfect loop
#pragma HLS loop_flatten off
#pragma HLS LOOP_TRIPCOUNT min = c_size_min max = c_size_max
            burstbuffer[j] = a[i + j];
        }

    // calculate and write results to global memory, the sequential write in a for
    // loop can be inferred to a memory burst access
    calc_write:
        for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size_max max = c_chunk_sz
            burstbuffer[j] = burstbuffer[j] + inc_value;
            a[i + j] = burstbuffer[j];
        }
    }
}

// Content of called function
void write(A buf_in[NUM], A* a_out) {
WRITE:
    for (int k = 0; k < NUM; k++) {
        a_out[k] = buf_in[k];
    }
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}