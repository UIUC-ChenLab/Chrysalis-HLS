#define DATA_SIZE 2048

void vadd(const unsigned int* in1, // Read-Only Vector 1
          const unsigned int* in2, // Read-Only Vector 2
          unsigned int* out_r,     // Output Result
          int size                 // Size in integer
          ) {
    unsigned int v1_buffer[BUFFER_SIZE];   // Local memory to store vector1
    unsigned int v2_buffer[BUFFER_SIZE];   // Local memory to store vector2
    unsigned int vout_buffer[BUFFER_SIZE]; // Local Memory to store result

#pragma HLS INTERFACE m_axi port = in1 depth = DATA_SIZE
#pragma HLS INTERFACE m_axi port = in2 depth = DATA_SIZE
#pragma HLS INTERFACE m_axi port = out_r depth = DATA_SIZE
    // Per iteration of this loop perform BUFFER_SIZE vector addition
    for (int i = 0; i < size; i += BUFFER_SIZE) {
#pragma HLS LOOP_TRIPCOUNT min = c_size / c_chunk_sz max = c_size / c_chunk_sz
        int chunk_size = BUFFER_SIZE;
        // boundary checks
        if ((i + BUFFER_SIZE) > size) chunk_size = size - i;

    // burst read of v1 and v2 vector from global memory
    read1:
        for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_chunk_sz max = c_chunk_sz
            v1_buffer[j] = in1[i + j];
        }
    read2:
        for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_chunk_sz max = c_chunk_sz
            v2_buffer[j] = in2[i + j];
        }

    // FPGA implementation, local array is mostly implemented as BRAM Memory
    // block.
    // BRAM Memory Block contains two memory ports which allow two parallel access
    // to memory. To utilized both ports of BRAM block, vector addition loop is
    // unroll with factor of 2. It is equivalent to following code:
    //  for (int j = 0 ; j < chunk_size ; j+= 2){
    //    vout_buffer[j]   = v1_buffer[j] + v2_buffer[j];
    //    vout_buffer[j+1] = v1_buffer[j+1] + v2_buffer[j+1];
    //  }
    // Which means two iterations of loop will be executed together and as a
    // result
    // it will double the performance.
    // Auto-pipeline is going to apply pipeline to this loop
    vadd:
        for (int j = 0; j < chunk_size; j++) {
// As the outer loop is not a perfect loop
#pragma HLS loop_flatten off
#pragma HLS UNROLL FACTOR = 2
#pragma HLS LOOP_TRIPCOUNT min = c_chunk_sz max = c_chunk_sz
            // perform vector addition
            vout_buffer[j] = v1_buffer[j] + v2_buffer[j];
        }

    // burst write the result
    write:
        for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_chunk_sz max = c_chunk_sz
            out_r[i + j] = vout_buffer[j];
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