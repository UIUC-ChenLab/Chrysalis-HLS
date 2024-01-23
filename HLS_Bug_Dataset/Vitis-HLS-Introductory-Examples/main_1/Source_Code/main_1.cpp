
#define DATA_SIZE 4096

int main(int argc, char** argv) {

    // Read settings
    std::string binaryFile = argv[1];
    int device_index = 0;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    // Vector Addition Kernel
    std::cout << "\nStarting the vadd kernel...\n";
    auto krnl_vadd = xrt::kernel(device, uuid, "krnl_vadd");

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bo0 = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(0));
    auto bo1 = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(1));
    auto bo_out = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(2));

    // Map the contents of the buffer object into host memory
    auto bo0_map = bo0.map<int*>();
    auto bo1_map = bo1.map<int*>();
    auto bo_out_map = bo_out.map<int*>();

    // Create the test data
    int buf_ref[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo0_map[i] = i;
        bo1_map[i] = i;
        buf_ref[i] = bo0_map[i] + bo1_map[i];
    }

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    bo0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo1.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Execution of the vadd kernel\n";
    auto run = krnl_vadd(bo0, bo1, bo_out, DATA_SIZE);
    run.wait();

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Validate our results
    if (std::memcmp(bo_out_map, buf_ref, DATA_SIZE))
        throw std::runtime_error("Value read back does not match reference");

    // Vector Multiplication Kernel
    std::cout << "\nStarting the vmult kernel...\n";
    auto krnl_vmult = xrt::kernel(device, uuid, "krnl_vmult");

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bo2 = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(0));
    auto bo3 = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(1));
    auto bo_output = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(2));

    // Map the contents of the buffer object into host memory
    auto bo2_map = bo2.map<int*>();
    auto bo3_map = bo3.map<int*>();
    auto bo_output_map = bo_output.map<int*>();

    // Create the test data
    int buf_ref1[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo2_map[i] = i;
        bo3_map[i] = i;
        buf_ref1[i] = bo2_map[i] * bo3_map[i];
    }

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    bo2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo3.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Execution of the vmult kernel\n";
    auto run1 = krnl_vmult(bo2, bo3, bo_output, DATA_SIZE);
    run1.wait();

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    bo_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Validate our results
    if (std::memcmp(bo_output_map, buf_ref1, DATA_SIZE))
        throw std::runtime_error("Value read back does not match reference");

    std::cout << "TEST PASSED\n";
    return 0;
}

// Content of called function
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
void store(hls::stream<vecOf16Words>& in, vecOf16Words* out, int size) {
Loop_St:
    for (int i = 0; i < size; i++) {
#pragma HLS performance target_ti = 32
#pragma HLS LOOP_TRIPCOUNT max = 32
        out[i] = in.read();
    }
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}