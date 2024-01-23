void parseBlockGenFSETable(hls::stream<bool>& blockValidStream,
                           hls::stream<uint32_t>& blockMetaStream,
                           hls::stream<ap_uint<IN_BYTES * 8> >& zstdInStream,
                           hls::stream<bool>& litValidStream,
                           hls::stream<uint32_t>& litMetaStream,
                           hls::stream<uint32_t>& fseTableLitStream,
                           hls::stream<ap_uint<IN_BYTES * 8> >& litDecodeInStream,
                           hls::stream<bool>& seqValidStream,
                           hls::stream<uint32_t>& seqMetaStream,
                           hls::stream<uint32_t>& fseTableSeqStream,
                           hls::stream<ap_uint<IN_BYTES * 8> >& seqDecodeInStream) {
    // parse blocks and decompress them
    const uint8_t c_inputByte = IN_BYTES;
    const uint8_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const uint16_t c_accRegWidthx3 = c_streamWidth * 3;

    uint8_t defDistOffsets[3] = {0, c_maxCharLit + 1, c_maxCharLit + c_maxCharDefOffset + 2};
    uint8_t maxCharLOM[3] = {c_maxCharLit, c_maxCharOffset, c_maxCharMatchlen};
    int16_t prevDistribution[3 * 64];
    uint8_t prevAccLog[3] = {6, 5, 6};
    xfSymbolCompMode_t prevFseMode[3] = {FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE};

parseBlock_main_loop:
    while (blockValidStream.read()) {
        // parse valid block and generate FSE tables for them
        xfBlockType_t blockType;
        uint32_t blockMeta = blockMetaStream.read(); // <blockType 8-bits><blockSize 24-bits>

        blockType = (xfBlockType_t)(blockMeta & 0x000000FF);
        uint32_t blockSize = blockMeta >> 8;
        // enable literal and decoding
        litValidStream << 1;
        seqValidStream << 1;

        // write common literal and sequence meta data
        litMetaStream << blockMeta;
        seqMetaStream << blockMeta;
        // write literal data
        if (blockType == RLE_BLOCK) {
            // write RLE byte
            ap_uint<c_streamWidth> tbuf = zstdInStream.read();
            tbuf &= (ap_uint<c_streamWidth>)(((uint64_t)1 << 8) - 1);
            litDecodeInStream << tbuf;
        } else if (blockType == RAW_BLOCK) {
        // write complete block data as it is
        block_write_rawblocklit_data:
            for (uint32_t i = 0; i < blockSize; i += c_inputByte) {
#pragma HLS PIPELINE II = 1
                ap_uint<c_streamWidth> tbuf = zstdInStream.read();
                litDecodeInStream << tbuf;
            }
        } else {
            // decompress zstd compressed blocks
            /*
             * Zstd compressed block contains Literals and Sequences sections
             *
             * Literals Section:
             *     <Literals_Section_Header><Huffman_Tree_Description><Jump_Table><Stream1><Stream2><Stream3><Stream4>
             * Sequences Section:
             *     <Sequences_Section_Header><Literals_Length_Table><Offset_Table><Match_Length_Table><bitStream>
             */
            uint32_t regeneratedSize = 0; // literal count
            uint32_t compressedSize = 0;
            bool quadStream;
            uint32_t remBlockSize = blockSize; // block header is not included in block size

            /*
             * Literal_Section_Header
             * <Literals_Block_Type><Size_Format><Regenerated_Size><Compressed_Size>
             *          2 bits              1-2 bits         5-20 bits            0-18 bits
             */
            // read a word
            ap_uint<c_accRegWidth> accRegister;
            uint8_t bytesInAcc = 0;
            uint8_t bitsInAcc = 0;
            uint8_t shiftCnt = 0;
            for (uint8_t i = 0; i < 2; i++) {
                accRegister.range((i + 1) * c_streamWidth - 1, i * c_streamWidth) = zstdInStream.read();
                bytesInAcc += IN_BYTES;
                bitsInAcc += c_streamWidth;
            }

            // Get Literals_Block_Type
            xfLitBlockType_t litBlockType = (xfLitBlockType_t)((uint8_t)accRegister.range(1, 0));
            shiftCnt += 2;

            // Read size format
            uint8_t sizeFormat = ((uint8_t)accRegister.range(3, 2));
            uint8_t regSizeBits = 0;
            bool getCompSize;
            uint8_t ebn = 0; // extra bytes needed
            // read regenerated and compressed size
            getCompSize = (litBlockType == CMP_LBLOCK || litBlockType == TREELESS_LBLOCK);
            if (getCompSize) {
                shiftCnt += 2;
                ebn = 2;
                switch (sizeFormat) {
                    case 0:
                        regSizeBits = 10;
                        quadStream = false;
                        break;
                    case 1:
                        regSizeBits = 10;
                        quadStream = true;
                        break;
                    case 2:
                        regSizeBits = 14;
                        quadStream = true;
                        ++ebn;
                        break;
                    case 3:
                        regSizeBits = 18;
                        quadStream = true;
                        ebn += 2;
                        break;
                }
            } else {
                // get bits used by Regenerated_Size
                if ((sizeFormat & 1) == 0) {
                    regSizeBits = 5;
                    shiftCnt += 1;
                } else {
                    shiftCnt += 2;
                    if (sizeFormat == 1) {
                        regSizeBits = 12;
                        ++ebn;
                    } else { // remaining option is 3
                        regSizeBits = 20;
                        ebn += 2;
                    }
                }
                quadStream = false;
            }

            // read regenerated size
            regeneratedSize = (accRegister.range(63, shiftCnt) & (((uint32_t)1 << regSizeBits) - 1));
            shiftCnt += regSizeBits;
            // read compressed size
            if (getCompSize) {
                compressedSize = (accRegister.range(63, shiftCnt) & (((uint32_t)1 << regSizeBits) - 1));
                shiftCnt += regSizeBits;
            }
            // will be byte aligned compulsorily
            bitsInAcc -= shiftCnt;
            accRegister >>= shiftCnt;
            bytesInAcc = bitsInAcc >> 3;
            remBlockSize -= (ebn + 1);

            // Literal metadata format
            // Word 0 is already written, which is default
            // Word 1 <litBlockType><quadStream><regeneratedSize>
            //           7-bits         1-bit       3 bytes
            // Word 2 <huffmanHeader><compressedSize>
            //           1 byte         3 bytes
            // FSETables if present
            uint32_t lmbuf = (uint8_t)litBlockType;
            if (quadStream) lmbuf += (1 << 7);
            lmbuf += (regeneratedSize << 8);
            litMetaStream << lmbuf; // write word 1

            // write literals
            uint32_t litSize2Write = regeneratedSize;
            bool noTree = false;
            switch (litBlockType) {
                case RLE_LBLOCK: {
                    uint8_t rleLit;
                    if (bytesInAcc == 0) {
                        accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                        bytesInAcc += c_inputByte;
                    }
                    rleLit = ((uint8_t)accRegister & 0XFF);
                    accRegister >>= 8;
                    --bytesInAcc;
                    ap_uint<c_streamWidth> rlbuf = rleLit;
                    litDecodeInStream << rlbuf;

                    bitsInAcc = bytesInAcc * 8;
                    remBlockSize -= 1;
                    break;
                }
                case TREELESS_LBLOCK: {
                    noTree = true;
                }
                case CMP_LBLOCK: {
                    litSize2Write = compressedSize;
                    // compressed literal block, FSE decode + huffman tree generation
                    // read from input if needed
                    if (bytesInAcc == 0) {
                        accRegister = zstdInStream.read();
                        bytesInAcc += c_inputByte;
                    }
                    uint8_t hufHeader = 0;
                    if (noTree) {
                        // Word 2 <huffmanHeader><compressedSize>
                        //           1 byte         3 bytes
                        litMetaStream << (compressedSize << 8);
                    } else {
                        hufHeader = accRegister;
                        accRegister >>= 8;
                        --bytesInAcc;
                        --litSize2Write;
                        --remBlockSize;
                        // Word 2 <huffmanHeader><compressedSize>
                        //           1 byte         3 bytes
                        litMetaStream << (compressedSize << 8) + hufHeader; // write word 2

                        // check if FSE decoding is required
                        if (hufHeader < 128) {
                            // FSE table generation and decoding before huffman decoding
                            uint8_t litAccuracyLog = 6; // max is 6 bits, will be modified as per input
                            // create FSE tables
                            int ret = generateFSETable<IN_BYTES>(fseTableLitStream, zstdInStream, accRegister,
                                                                 bytesInAcc, litAccuracyLog, c_maxCharHuffman,
                                                                 FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE,
                                                                 c_defaultDistribution, prevDistribution);
                            litSize2Write -= ret;
                            remBlockSize -= ret;
                            // send relevant metadata
                            // Word 3 <accuracyLog><byteUsedByFSE>
                            //          1 byte          1 byte
                            litMetaStream << litAccuracyLog + ((uint32_t)ret << 8); // write word 3
                        }
                    }
                    bitsInAcc = bytesInAcc * 8;
                }
                case RAW_LBLOCK: {
                    // Bigger register needed, because more bytes than input bytes can be present in the accumulator
                    ap_uint<c_accRegWidthx3> wbuf = accRegister;
                    uint8_t bytesWritten = 0;
                    uint8_t updBInAcc = bytesInAcc;

                    bitsInAcc = bytesInAcc * 8;
                // write block data
                cmplit_write_block:
                    for (int i = 0; i < litSize2Write; i += c_inputByte) {
#pragma HLS PIPELINE II = 1
                        if (i < (const int)(litSize2Write - bytesInAcc)) {
                            wbuf.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                            updBInAcc += c_inputByte;
                        }
                        ap_uint<c_streamWidth> tmpV = wbuf;
                        litDecodeInStream << tmpV;

                        if (i > (const int)(litSize2Write - c_inputByte)) {
                            bytesWritten = litSize2Write - i;
                        } else {
                            wbuf >>= c_streamWidth;
                            updBInAcc -= c_inputByte;
                        }
                    }
                    accRegister = (wbuf >> (bytesWritten * 8));
                    bytesInAcc = updBInAcc - bytesWritten;
                    bitsInAcc = bytesInAcc * 8;
                    remBlockSize -= litSize2Write;
                } break;
                default:
                    // must not reach here
                    break;
            }
            // decode zstd sequence header
            /*
             * Sequences Section:
             *     <Sequences_Section_Header><Literals_Length_Table><Offset_Table><Match_Length_Table><bitStream>
             *
             * Sequences_Section_Header
             * <Number_of_Sequences><Symbol_Compression_Modes>
             *       1-3 bytes              1 byte
             */
            if (bytesInAcc < remBlockSize && bytesInAcc < 4) {
                accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                bytesInAcc += c_inputByte;
            }
            uint8_t byteCnt = 0;
            uint32_t seqCnt = 0;
            xfSymbolCompMode_t literalsLengthMode;
            xfSymbolCompMode_t offsetsMode;
            xfSymbolCompMode_t matchLengthsMode;
            // read 1 byte and check number of sequences
            uint8_t byte_0 = (uint8_t)(accRegister & 0x000000FF);
            accRegister >>= 8;
            --bytesInAcc;
            ++byteCnt;

            if (byte_0 == 0) {
                // there are no sequences, sequence section stops here.
                // Decompressed content is defined as Literals Section content.
                // The FSE tables used in Repeat_Mode are not updated.
                remBlockSize -= byteCnt;
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <1> if further decoding needed
                // Word 2 <literalCount>
                seqMetaStream << 0;               // Word 1
                seqMetaStream << regeneratedSize; // Word 2
            } else {
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <decode_flag> 1 if further decoding needed, else 0
                // Word 2 <literalCount>
                seqMetaStream << 1;               // Word 1
                seqMetaStream << regeneratedSize; // Word 2

                if (byte_0 > 0 && byte_0 < 128) {
                    // uses one byte
                    seqCnt = byte_0;
                } else if (byte_0 > 127 && byte_0 < 255) {
                    // uses 2 bytes
                    ++byteCnt;
                    // calculate Number_of_Sequences = ((byte0-128) << 8) + byte1
                    seqCnt = ((((uint32_t)byte_0 - 128) << 8) + ((uint32_t)accRegister & 0xFF));
                    accRegister >>= 8;
                    --bytesInAcc;
                } else {
                    // uses 3 bytes, dump first byte, need next 2 bytes
                    // calculate Number_of_Sequences = byte1 + (byte2<<8) + 0x7F00
                    seqCnt = (((uint32_t)accRegister & 0x0000FFFF) + 32512);
                    byteCnt += 2;
                    accRegister >>= 16;
                    bytesInAcc -= 2;
                }
                // Get symbol compression mode
                byte_0 = (uint8_t)accRegister;
                accRegister >>= 8;
                --bytesInAcc;
                bitsInAcc = bytesInAcc * 8;
                ++byteCnt;

                matchLengthsMode = (xfSymbolCompMode_t)((byte_0 >> 2) & 3);
                offsetsMode = (xfSymbolCompMode_t)((byte_0 >> 4) & 3);
                literalsLengthMode = (xfSymbolCompMode_t)((byte_0 >> 6) & 3);

                remBlockSize -= byteCnt;

                // generate FSE tables
                ap_uint<c_accRegWidth> fseAcc = accRegister;
                xfSymbolCompMode_t modeLOM[3] = {literalsLengthMode, offsetsMode, matchLengthsMode};
                uint8_t accuracyLog[3] = {6, 5, 6}; // for default distribution, overwritten for fse compressed ones
#pragma HLS ARRAY_PARTITION variable = modeLOM complete
#pragma HLS ARRAY_PARTITION variable = accuracyLog complete

                if (offsetsMode == PREDEFINED_MODE)
                    maxCharLOM[1] = c_maxCharDefOffset;
                else if (offsetsMode == FSE_COMPRESSED_MODE)
                    maxCharLOM[1] = c_maxCharOffset;

            gen_lom_fse_tables:
                for (uint8_t i = 0; i < 3; ++i) {
                    // Generated FSE tables for literal lengths, offsets and match lengths
                    if (modeLOM[i] == REPEAT_MODE) accuracyLog[i] = prevAccLog[i];
                    int ret = generateFSETable<IN_BYTES>(
                        fseTableSeqStream, zstdInStream, fseAcc, bytesInAcc, accuracyLog[i], maxCharLOM[i], modeLOM[i],
                        prevFseMode[i], &(c_defaultDistribution[defDistOffsets[i]]), &(prevDistribution[i * 64]));
                    prevFseMode[i] = modeLOM[i];
                    prevAccLog[i] = accuracyLog[i];
                    remBlockSize -= ret;
                }
                accRegister = fseAcc;
                bitsInAcc = bytesInAcc * 8;
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <decode_flag> 1 if further decoding needed, else 0
                // Word 2 <literalCount>
                // Word 3 <symbolCompressionMode><remBlockSize>
                //              1 byte              3 bytes
                // Word 4 <seqCnt>
                // Word 5 <AccuracyLogs> --> <litlen><offset><matlen> 1 byte each lower-higher
                uint32_t sqmbuf = (remBlockSize << 8) + byte_0;
                seqMetaStream << sqmbuf; // Word 3
                seqMetaStream << seqCnt; // Word 4
                sqmbuf = (((uint32_t)accuracyLog[2] << 16) + ((uint32_t)accuracyLog[1] << 8) + accuracyLog[0]);
                seqMetaStream << sqmbuf; // Word 5
                // send the remaining block data, sequence bitstream

                int readBytes = remBlockSize - bytesInAcc;
                ap_uint<c_accRegWidthx3> wbuf = accRegister;
                bitsInAcc = bytesInAcc * 8;
            // write block data
            cmpseq_write_block:
                for (int i = 0; i < remBlockSize; i += c_inputByte) {
#pragma HLS PIPELINE II = 1
                    if (i < readBytes) {
                        wbuf.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                    }
                    ap_uint<c_streamWidth> tmpV = wbuf;
                    seqDecodeInStream << tmpV;

                    wbuf >>= c_streamWidth;
                }
            }
        }
    }
    litValidStream << 0;
    seqValidStream << 0;
}