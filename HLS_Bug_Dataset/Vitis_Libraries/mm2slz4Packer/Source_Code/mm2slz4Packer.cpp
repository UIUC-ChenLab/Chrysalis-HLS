void mm2slz4Packer(const ap_uint<DATAWIDTH>* in,
                   ap_uint<DATAWIDTH>* orig_input_data,
                   hls::stream<ap_uint<DATAWIDTH> >& outStream,
                   hls::stream<uint32_t>& outStreamSize,
                   uint32_t* compressd_size,
                   uint32_t* in_block_size,
                   uint32_t no_blocks,
                   uint32_t block_size_in_kb,
                   uint32_t xxhashVal,
                   uint32_t input_size) {
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;

    ap_uint<DATAWIDTH> headerData = 0;

    // LZ4 Header Data
    headerData(7, 0) = xxhashVal;
    headerData = headerData << 64;
    headerData(63, 0) = input_size;
    headerData = headerData << 8;
    headerData(7, 0) = block_size_in_kb;
    headerData = headerData << 8;
    headerData(7, 0) = xf::compression::FLG_BYTE;
    headerData = headerData << 8;
    headerData(7, 0) = xf::compression::MAGIC_BYTE_4;
    headerData = headerData << 8;
    headerData(7, 0) = xf::compression::MAGIC_BYTE_3;
    headerData = headerData << 8;
    headerData(7, 0) = xf::compression::MAGIC_BYTE_2;
    headerData = headerData << 8;
    headerData(7, 0) = xf::compression::MAGIC_BYTE_1;

    // Handle header or residue here
    uint32_t block_stride = block_size_in_kb * 1024 / 64;

    uint32_t blkCompSize = 0;
    uint32_t origSize = 0;
    uint32_t sizeInWord = 0;
    uint32_t cBlen = 4;
    uint32_t hSize = 15;
    outStreamSize << hSize;
    outStream << headerData;

    // Run over number of blocks
    for (int bIdx = 0; bIdx < no_blocks; bIdx++) {
        blkCompSize = compressd_size[bIdx];
        origSize = in_block_size[bIdx];
        // Put compress block & input block
        // into streams for next block
        sizeInWord = (blkCompSize - 1) / c_word_size + 1;

        // Send size in bytes
        outStreamSize << cBlen;
        outStream << blkCompSize;
        outStreamSize << blkCompSize;

        // Copy data from global memory to local
        // Put it into stream
        for (uint32_t i = 0; i < sizeInWord; i += BURST_SIZE) {
            uint32_t chunk_size = BURST_SIZE;

            if (i + BURST_SIZE > sizeInWord) chunk_size = sizeInWord - i;

            if (blkCompSize == origSize) {
            memrd1:
                for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                    outStream << orig_input_data[block_stride * bIdx + i + j];
                }
            } else {
            memrd2:
                for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                    outStream << in[block_stride * bIdx + i + j];
                }
            }
        }
    }
    outStreamSize << 0;
}