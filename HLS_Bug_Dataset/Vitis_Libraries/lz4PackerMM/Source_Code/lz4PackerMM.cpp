void lz4PackerMM(ap_uint<DATAWIDTH>* orig_input_data,
                 ap_uint<DATAWIDTH>* in,
                 ap_uint<DATAWIDTH>* out,
                 uint32_t* in_block_size,
                 uint32_t* compressd_size,
                 uint32_t* packerCompressedSize,
                 uint32_t xxhashVal,
                 uint32_t block_length,
                 uint32_t no_blocks,
                 uint32_t input_size) {
#pragma HLS DATAFLOW
    hls::stream<ap_uint<DATAWIDTH> > inStreamV("inStreamV");
    hls::stream<uint32_t> inStreamVSize("inStreamVSize");
    hls::stream<uint32_t> outStreamVSize("inStreamVSize");
    hls::stream<ap_uint<DATAWIDTH> > outStreamV("outStreamV");
    hls::stream<bool> outStreamVEos("outStreamVEos");

#pragma HLS STREAM variable = inStreamV depth = 32
#pragma HLS STREAM variable = inStreamVSize depth = 32
#pragma HLS STREAM variable = outStreamVSize depth = 32
#pragma HLS STREAM variable = outStreamV depth = 32
#pragma HLS STREAM variable = outStreamVEos depth = 32

#pragma HLS BIND_STORAGE variable = inStreamV type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

    xf::compression::mm2slz4Packer<DATAWIDTH, BURST_SIZE>(in, orig_input_data, inStreamV, inStreamVSize, compressd_size,
                                                          in_block_size, no_blocks, block_length, xxhashVal,
                                                          input_size);
    xf::compression::blockPacker<DATAWIDTH>(inStreamV, inStreamVSize, outStreamV, outStreamVEos, outStreamVSize);
    xf::compression::details::s2mmEosSimple<DATAWIDTH, BURST_SIZE>(out, outStreamV, outStreamVEos, outStreamVSize,
                                                                   packerCompressedSize);
}

// Content of called function
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

// Content of called function
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