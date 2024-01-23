
#define totalNumWords 512

int main(int argc, char** argv) {
    unsigned int device_index = 0;
    std::string binaryFile = argv[1];

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Opened the device" << device_index << std::endl;

    auto uuid = device.load_xclbin(binaryFile);

    size_t vector_size_bytes = sizeof(int) * totalNumWords;

    auto krnl = xrt::kernel(device, uuid, "diamond");

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bufIn = xrt::bo(device, vector_size_bytes, krnl.group_id(0));
    auto bufOut = xrt::bo(device, vector_size_bytes, krnl.group_id(1));

    // Map the contents of the buffer object into host memory
    auto bufIn_map = bufIn.map<int*>();
    auto bufOut_map = bufOut.map<int*>();
    std::fill(bufIn_map, bufIn_map + totalNumWords, 0);
    std::fill(bufOut_map, bufOut_map + totalNumWords, 0);

    // Initialize the input data
    for (int i = 0; i < totalNumWords; i++)
        bufIn_map[i] = (uint32_t)i;
    std::cout << "The Test Data initialized" << std::endl;

    // Create the reference golden data for comparison
    int bufReference[totalNumWords];
    for (int i = 0; i < totalNumWords; ++i) {
        bufReference[i] = ((i * 3) + 25) + ((i * 3) * 2);
    }
    std::cout << "The Test Data created" << std::endl;

    std::cout << "Execution of the kernel\n";
    xrt::run run[3];

    for (int i = 0; i < 3; i++) {
        std::cout << "synchronize input buffer data to device global memory\n";
        bufIn.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        run[i] = krnl(bufIn, bufOut, totalNumWords / 16);
        run[i].wait();
        std::cout << "Get the output data from the device" << std::endl;
        bufOut.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

#if debug
    for (int i = 0; i < totalNumWords; i++) {
        std::cout << "Referece  " << bufReference[i] << std::endl;
        std::cout << "Out  " << bufOut_map[i] << std::endl;
    }
#endif

    // Validate our results
    if (std::memcmp(bufOut_map, bufReference, totalNumWords))
        throw std::runtime_error("Value read back does not match reference");
    std::cout << "TEST PASSED\n";
    return 0;
}

// Content of called function
void read(A* a_in, A buf_out[NUM]) {
READ:
    for (int i = 0; i < NUM; i++) {
        buf_out[i] = a_in[i];
    }
}