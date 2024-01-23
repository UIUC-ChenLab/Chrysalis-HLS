void lz4HeaderProcessing(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                         hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                         hls::stream<dt_lz4BlockInfo>& blockInfoStream,
                         const uint32_t inputSize) {
    if (inputSize == 0) return;

    const int c_parallelBit = PARALLEL_BYTES * 8;
    ap_uint<3 * c_parallelBit> inputWindow;
    ap_uint<c_parallelBit> outStreamValue = 0;

    uint32_t readBytes = 0, processedBytes = 0;
    uint32_t origCompLen = 0, compLen = 0, blockSizeinKB = 0;
    uint8_t inputIdx = 0;
    bool outFlag = false;

    // to send both compressSize and storedBlock data
    dt_lz4BlockInfo blockInfo;

    for (uint8_t i = 0; i < 3; i++) {
#pragma HLS PIPELINE II = 1
        inputWindow.range((i + 1) * c_parallelBit - 1, i * c_parallelBit) = inStream.read();
        readBytes += PARALLEL_BYTES;
    }

    // Read magic header 4 bytes
    char magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
    for (uint32_t i = 0; i < MAGIC_HEADER_SIZE; i++) {
#pragma HLS PIPELINE II = 1
        int magicByte = (int)inputWindow.range((i + 1) * 8 - 1, i * 8);
        if (magicByte == magic_hdr[i])
            continue;
        else {
            assert(0);
        }
    }

    char c = (char)inputWindow.range(47, 40);
    switch (c) {
        case BSIZE_STD_64KB:
            blockSizeinKB = 64;
            break;
        case BSIZE_STD_256KB:
            blockSizeinKB = 256;
            break;
        case BSIZE_STD_1024KB:
            blockSizeinKB = 1024;
            break;
        case BSIZE_STD_4096KB:
            blockSizeinKB = 4096;
            break;
        default:
            assert(0);
    }

    uint32_t blockSizeInBytes = blockSizeinKB * 1024;

    inputIdx += 7;
    processedBytes += 8;

    for (; (processedBytes + inputIdx) < inputSize;) {
        if ((inputIdx >= PARALLEL_BYTES)) {
            inputWindow >>= c_parallelBit;
            if (readBytes < inputSize) {
                ap_uint<c_parallelBit> input = inStream.read();
                readBytes += PARALLEL_BYTES;
                inputWindow.range(3 * c_parallelBit - 1, 2 * c_parallelBit) = input;
            }
            inputIdx = inputIdx - PARALLEL_BYTES;
            processedBytes += PARALLEL_BYTES;
        }

        uint32_t chunkSize = 0;

        ap_uint<32> compressedSize = inputWindow >> (inputIdx * 8);
        inputIdx += 4;
        bool storedBlkFlg = (compressedSize.range(31, 31) == 1) ? true : false;

        uint32_t tmp = compressedSize;
        tmp >>= 24;

        if (tmp == 128) {
            uint8_t b1 = compressedSize;
            uint8_t b2 = compressedSize >> 8;
            uint8_t b3 = compressedSize >> 16;

            if (b3 == 1) {
                compressedSize = blockSizeInBytes;
            } else {
                uint16_t size = 0;
                size = b2;
                size <<= 8;
                uint16_t temp = b1;
                size |= temp;
                compressedSize = size;
            }
        }

        compLen = compressedSize;

        blockInfo.compressedSize = compressedSize;

        if (storedBlkFlg) {
            blockInfo.storedBlock = 1;
        } else {
            blockInfo.storedBlock = 0;
        }
        // write compress length to outSizeStream
        blockInfoStream << blockInfo;

        uint32_t len = compLen;
        for (uint32_t blockData = 0; blockData < compLen; blockData += PARALLEL_BYTES) {
#pragma HLS PIPELINE II = 1
            if ((inputIdx >= PARALLEL_BYTES)) {
                inputWindow >>= c_parallelBit;
                if (readBytes < inputSize) {
                    ap_uint<c_parallelBit> input = inStream.read();
                    readBytes += PARALLEL_BYTES;
                    inputWindow.range(3 * c_parallelBit - 1, 2 * c_parallelBit) = input;
                }
                inputIdx = inputIdx - PARALLEL_BYTES;
                processedBytes += PARALLEL_BYTES;
            }

            outStreamValue = inputWindow >> (inputIdx * 8);
            outStream << outStreamValue;

            if (len >= PARALLEL_BYTES) {
                inputIdx += PARALLEL_BYTES;
                len -= PARALLEL_BYTES;
            } else {
                inputIdx += len;
                len = 0;
            }
        }
    }
    blockInfo.compressedSize = 0;
    // writing 0 to indicate end of data
    blockInfoStream << blockInfo;
}