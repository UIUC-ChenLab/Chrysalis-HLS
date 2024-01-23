void blockPacker(hls::stream<ap_uint<DATAWIDTH> >& inStream,
                 hls::stream<uint32_t>& inStreamSize,
                 hls::stream<ap_uint<DATAWIDTH> >& outStream,
                 hls::stream<bool>& outStreamEos,
                 hls::stream<uint32_t>& outCompressedSize) {
    // Main Module Starts
    // Local Variable Declaration
    const int c_byteSize = 8;
    uint32_t lbuf_idx = 0;
    uint32_t index = 0;
    uint32_t prodLbuf = 0;
    uint32_t endSizeCnt = 0;

    // Parallel Byte Variable
    uint32_t c_parallelByte = DATAWIDTH / c_byteSize;

    // Local Buffer for shift operation
    ap_uint<DATAWIDTH* 2> lcl_buffer = 0;

// Loop for processing each compressed size block
size_stream_loop:
    for (uint32_t size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // Increment the size variable.
        endSizeCnt += size;

        // Calculation for byte processing
        uint32_t leftBytes = size % c_parallelByte;
        uint32_t alignedBytes = size - leftBytes;

        // One-time multiplication calculation for index
        prodLbuf = lbuf_idx * c_byteSize;

    loop_aligned:
        for (uint32_t i = 0; i < alignedBytes; i += c_parallelByte) {
#pragma HLS PIPELINE II = 1
            // Reading Input Data
            lcl_buffer.range(prodLbuf + DATAWIDTH - 1, prodLbuf) = inStream.read();

            // Writing output into memory
            outStream << lcl_buffer.range(DATAWIDTH - 1, 0);
            outStreamEos << 0;
            // Shifting by Global datawidth
            lcl_buffer >>= DATAWIDTH;
        }

        // Left bytes from each block
        if (leftBytes) {
            lcl_buffer.range(prodLbuf + DATAWIDTH - 1, prodLbuf) = inStream.read();
            lbuf_idx += leftBytes;
        }

        // Check for PARALLEL BYTE data and write to output stream
        if (lbuf_idx >= c_parallelByte) {
            outStream << lcl_buffer.range(DATAWIDTH - 1, 0);
            outStreamEos << 0;
            lcl_buffer >>= DATAWIDTH;
            lbuf_idx -= c_parallelByte;
        }
    }

    // Left Over Data Handling
    if (lbuf_idx) {
        lcl_buffer.range(DATAWIDTH - 1, (lbuf_idx * c_byteSize)) = 0;
        outStream << lcl_buffer.range(DATAWIDTH - 1, 0);
        outStreamEos << 0;
        lbuf_idx = 0;
    }

    // Dummy data
    outStreamEos << 1;
    outStream << 0;

    // Total Size
    outCompressedSize << endSizeCnt;
}