void parseFramesAndBlocks(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                          hls::stream<ap_uint<4> >& inStrobe,
                          hls::stream<bool>& blockValidStream,
                          hls::stream<uint32_t>& blockMetaStream,
                          hls::stream<ap_uint<IN_BYTES * 8> >& zstdBlockStream) {
    // decompress all zstd frames
    const uint8_t c_parallelByte = IN_BYTES;
    const uint8_t c_parallelBit = c_parallelByte * 8;

    // 14 because of magic number + frame header
    const uint8_t numWidth = 14 / c_parallelByte + 1;
    ap_uint<128> inputWindow;
    bool lastInputWord = false;
    uint8_t inputIdx = 0;
    ap_uint<4> istb;
    uint32_t readBytes = 0;
    uint32_t nextFrameData = 0;

// parse all frames and exit when there is no data to read in inStream
parseFrames_main_loop:
    while (!lastInputWord) {
        uint32_t magicNumber;
        uint64_t windowSize = 0;
        uint32_t dictionaryId = 0;
        uint64_t frameContentSize = 0;
        uint32_t contentCheckSum = 0;
        uint32_t blockMaxSize = 0;
        uint32_t processedBytes = 0;
        istb = 1;

        for (uint8_t i = readBytes; i < (numWidth * c_parallelByte); i += c_parallelByte) {
#pragma HLS PIPELINE II = 1
            if (istb == 0) { // 0 data here means data ends here and there is no more frames to decode
                lastInputWord = true;
            } else {
                istb = inStrobe.read();
                inputWindow.range((i + c_parallelByte) * 8 - 1, i * 8) = inStream.read();
                readBytes += c_parallelByte;
            }
        }

        if (lastInputWord) { // 0 data here means data ends here and there is no more frames to decode
            continue;
        }

        /* Frame format
         * <Magic_Number><Frame_Header><Data_Block(s).....><Content_Checksum>
         * 	  4 bytes      2-14 bytes      n bytes....          0-4 bytes
         */
        // printf("Bytes in Acc: %d\n", bytesInAcc);
        // Read data to accumulator
        magicNumber = inputWindow.range(31, 0);
        inputIdx += 4;

        // verify magic number
        if (magicNumber != c_magicNumber) {
            if ((magicNumber & c_skippableFrameMask) != c_skipFrameMagicNumber) {
                // Error: Invalid Frame, magic number mismatch !!
                return;
            } else {
                uint32_t frameSize = inputWindow >> inputIdx * 8;
                inputIdx += 4;
                processedBytes += 4;

                if ((readBytes - processedBytes) > frameSize) {
                    inputIdx += frameSize;
                } else {
                    uint32_t skfRemBytes = frameSize - (readBytes - processedBytes);
                    uint32_t iterLim = 1 + ((skfRemBytes - 1) / c_parallelByte);
                    // skip this frame
                    ap_uint<c_parallelBit> input;
                skip_frame_loop:
                    for (uint32_t i = 0; i < iterLim; ++i) {
#pragma HLS PIPELINE II = 1
                        istb = inStrobe.read();
                        input = inStream.read();
                    }
                    uint32_t remBytes = (c_parallelByte * iterLim) - skfRemBytes;
                    nextFrameData = (c_parallelByte - remBytes);
                    input >>= remBytes * 8;
                    inputWindow.range(nextFrameData * 8 - 1, 0) = input;
                    inputIdx = 0;
                    readBytes = nextFrameData;
                }
            }
            continue;
            //} else {
            //  nextFrameData = 0;
        }
        /* Frame_Header format
         * <Frame_Header_Descriptor><Window_Descriptor><Dictionary_Id><Frame_Content_Size>
         * 		  1 byte				  0-1 byte		  0-4 bytes
         * 0-8
         * bytes
         */
        // Read data to accumulator
        ap_uint<8> headerDesc = inputWindow.range(39, 32);
        inputIdx += 1;

        // Parse frame header descriptor
        /*
         * Frame_Header_Descriptor
         * <Frame_Content_Size_flag><Single_Segment_flag><Not used><Content_Checksum_flag><Dictionary_ID_flag>
         *          bit 7-6                 bit 5         bit 4-3           bit 2               bit 1-0
         */
        bool checksumFlag = headerDesc.range(2, 2);
        bool singleSegmentFlag = headerDesc.range(5, 5);
        uint8_t didFieldSize = headerDesc.range(1, 0);
        uint8_t fcsFieldSize = headerDesc.range(7, 6);

        didFieldSize += (uint8_t)(didFieldSize == 3);
        fcsFieldSize = fcsFieldSize ? ((uint8_t)1 << fcsFieldSize) : (fcsFieldSize | singleSegmentFlag);
        // Parse window descriptor
        /*
         * Calculate minimum buffer memory size (Window_Size) required for decompression for this frame.
         * Minimum window size = 1KB and maximum Window_Size is ( (1 << 41) + 7 * (1 << 38) ) bytes, which is 3.75 TB.
         *
         * <Exponent><Mantissa>
         *  bit 7-3	  bits 2-0
         */
        if (!singleSegmentFlag) {
            // read 1 byte
            uint8_t winDesc = (uint8_t)(inputWindow.range(47, 40));
            inputIdx += 1;
            // get window size
            uint8_t windowLog = (10 + (winDesc >> 3));
            uint64_t windowBase = (uint64_t)1 << windowLog;
            uint64_t windowAdd = (windowBase >> 3) * (winDesc & 7);
            windowSize = windowBase + windowAdd;
        }

        // read Dictionary_Id
        // read bytes if needed
        dictionaryId = (didFieldSize > 0) ? inputWindow.range((inputIdx + didFieldSize) * 8 - 1, inputIdx * 8) : 0;
        inputIdx += didFieldSize;

        // read Frame_Content_Size
        frameContentSize = (fcsFieldSize > 0) ? inputWindow.range((inputIdx + fcsFieldSize) * 8 - 1, inputIdx * 8) : 0;
        frameContentSize += ((fcsFieldSize == 2) ? 256 : 0);
        inputIdx += fcsFieldSize;

        if (singleSegmentFlag) {
            windowSize = frameContentSize;
        }
        // Block_Max_Size is minimum of window size and 128K
        if (windowSize < (128 * 1024)) {
            blockMaxSize = windowSize;
        } else {
            blockMaxSize = 128 * 1024;
        }

        // parse Blocks in this Frame and pass block data to block decompression unit
        bool isLastBlock = false;

        inputWindow >>= (inputIdx * 8);
        uint8_t validBytes = readBytes - inputIdx;
        processedBytes += inputIdx;
        inputIdx = 0;

    parse_block_in_frame:
        while (!isLastBlock) {
            if (validBytes < 4) {
                istb = inStrobe.read();
                ap_uint<c_parallelBit> input = inStream.read();
                inputWindow.range((validBytes + c_parallelByte) * 8 - 1, validBytes * 8) = input;
                readBytes += c_parallelByte;
                validBytes += c_parallelByte;
            }

            // Parse block header
            /*
             * Block_Header: 3 bytes
             * <Last_Block><Block_Type><Block_Size>
             *    bit 0		 bits 1-2    bits 3-23
             */
            ap_uint<24> blockHeader = inputWindow.range(23, 0);
            processedBytes += 3;

            inputWindow >>= 24;
            validBytes -= 3;

            isLastBlock = (blockHeader & 1) ? true : false;
            xfBlockType_t blockType = (xfBlockType_t)(((uint8_t)blockHeader >> 1) & 3);
            uint32_t blockSize = (blockHeader >> 3);

            // enable processing for this block
            blockValidStream << 1;

            // write metadata to blockMetaStream
            uint32_t metabuf = (blockSize << 8) + ((uint8_t)blockType);
            blockMetaStream << metabuf;
            // copy blockSize data
            uint32_t bs2Write = blockSize;

            if (blockType == RLE_BLOCK) {
                bs2Write = 1;
            }

            uint32_t blockLen = bs2Write;
            int readCheck = bs2Write - validBytes;
            uint8_t updBInAcc = validBytes;
            uint8_t bytesWritten = 0;

        parser_write_block_data:
            for (int i = 0; i < bs2Write; i += c_parallelByte) {
#pragma HLS PIPELINE II = 1
                if (i < readCheck) {
                    istb = inStrobe.read();
                    inputWindow.range((validBytes + c_parallelByte) * 8 - 1, validBytes * 8) = inStream.read();
                    updBInAcc += c_parallelByte;
                }

                ap_uint<c_parallelBit> outValue = inputWindow;
                zstdBlockStream << outValue;

                if (i > (const int)(bs2Write - c_parallelByte)) {
                    bytesWritten = bs2Write - i;
                } else {
                    inputWindow >>= c_parallelBit;
                    updBInAcc -= c_parallelByte;
                }
            }
            validBytes = updBInAcc - bytesWritten;
            inputWindow >>= bytesWritten * 8;
        }

        // check for checksum flag
        if (checksumFlag) {
            if (validBytes < 4) {
                istb = inStrobe.read();
                ap_uint<c_parallelBit> input = inStream.read();
            }
        }
        inputIdx = 0;
        readBytes = validBytes;
    }
    blockValidStream << 0;
}