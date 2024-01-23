void decodeLiterals(hls::stream<bool>& litValidStream,
                    hls::stream<uint32_t>& litMetaStream,
                    hls::stream<uint32_t>& fseTableStream,
                    hls::stream<ap_uint<IN_BYTES * 8> >& litDecodeInStream,
                    hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    // decode literals and forward to literal stream
    const uint8_t c_inputByte = IN_BYTES;
    const uint8_t c_outByte = OUT_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const uint8_t c_factor = OUT_BYTES / IN_BYTES;
    uint8_t hufWeights[256];
    uint32_t litFseTable[256];

    uint8_t huffDecoderTableLog = 6;
    uint16_t wCnt = 0;
    IntVectorStream_dt<64, 1> outValue;

decodeLiterals_main_loop:
    while (litValidStream.read()) {
        // Literal metadata format
        // Word 0 is block metadata Word 0
        uint32_t litMeta = litMetaStream.read();
        xfBlockType_t blockType = (xfBlockType_t)(litMeta & 0x000000FF);
        ap_uint<LMO_WIDTH> blockSize = litMeta >> 8;
        // for raw and RLE Blocks forward as it is
        ap_uint<LMO_WIDTH> lbtWrite = blockSize;
        if (blockType == RLE_BLOCK) lbtWrite = 1;

        if (blockType == RLE_BLOCK || blockType == RAW_BLOCK) {
            // write complete block data as it is
            bool readFlag = (c_inputByte < lbtWrite) ? true : false;
            for (int i = 0; i < lbtWrite; readFlag = ((i + c_inputByte) < lbtWrite)) {
#pragma HLS PIPELINE II = 2
                ap_uint<64> lit = 0;
                lit.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                if (readFlag) lit.range(2 * c_streamWidth - 1, c_streamWidth) = litDecodeInStream.read();
                outValue.data[0] = lit;
                outValue.strobe = 1;
                literalStream << outValue;
                i += c_outByte;
            }
            outValue.strobe = 0;
            literalStream << outValue;
        } else {
            bool quadStream = false;
            xfLitBlockType_t litBlockType;
            ap_uint<LMO_WIDTH> regeneratedSize;
            ap_uint<LMO_WIDTH> compressedSize = 0;

            // literal metadata
            // Word 1 <litBlockType><quadStream><regeneratedSize>
            //           7-bits         1-bit       3 bytes
            litMeta = litMetaStream.read();
            litBlockType = (xfLitBlockType_t)((uint8_t)litMeta & 3);
            if (litMeta & 0x00000080) quadStream = true;
            regeneratedSize = litMeta >> 8;
            // write literals
            ap_uint<LMO_WIDTH> litSize2Write = regeneratedSize;
            if (litBlockType == RLE_LBLOCK) {
                // pass the RLE literal
                ap_uint<64> tbuf = litDecodeInStream.read();
                uint8_t rleLit = tbuf;
            decodelit_prep_rlelit_buff:
                for (uint8_t i = 1; i < c_outByte; ++i) {
#pragma HLS UNROLL
                    tbuf.range(((i + 1) * 8) - 1, i * 8) = rleLit;
                }
            decodelit_write_rlelit_data:
                for (ap_uint<LMO_WIDTH> i = 0; i < litSize2Write; i += c_outByte) {
#pragma HLS PIPELINE II = 1
                    outValue.data[0] = tbuf;
                    outValue.strobe = 1;
                    literalStream << outValue;
                }
                outValue.strobe = 0;
                literalStream << outValue;
            } else if (litBlockType == RAW_LBLOCK) {
                bool readFlag = (c_inputByte < litSize2Write) ? true : false;
                for (ap_uint<LMO_WIDTH> i = 0; i < litSize2Write; readFlag = ((i + c_inputByte) < litSize2Write)) {
#pragma HLS PIPELINE II = 2
                    ap_uint<64> lit = 0;
                    lit.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                    if (readFlag) lit.range(2 * c_streamWidth - 1, c_streamWidth) = litDecodeInStream.read();
                    outValue.data[0] = lit;
                    outValue.strobe = 1;
                    literalStream << outValue;
                    i += c_outByte;
                }
                outValue.strobe = 0;
                literalStream << outValue;
            } else {
                // Literal metadata
                // Word 2 <huffmanHeader><compressedSize>
                //           1 byte         3 bytes
                litMeta = litMetaStream.read();
                uint8_t hufHeader = (uint8_t)litMeta;
                uint8_t remBytesInFse = 0;
                compressedSize = litMeta >> 8;
                litSize2Write = compressedSize;

                ap_uint<c_accRegWidth> accHuff = 0;
                uint8_t bytesInAcc = 0;
                ap_uint<LMO_WIDTH> remBytes = 0;
                if (litBlockType == CMP_LBLOCK) {
                    uint8_t accuracyLog = 6;
                    uint16_t fwCnt = 0;
                    // init huffman weights
                    for (int i = 0; i < 256; i += 2) {
                        hufWeights[i] = 0;
                        hufWeights[i + 1] = 0;
                    }
                    if (hufHeader < 128) {
                        // read FSE table
                        uint32_t tblVCnt = fseTableStream.read();
                    lit_read_fse_table:
                        for (int i = 0; i < tblVCnt; ++i) {
#pragma HLS PIPELINE II = 1
                            litFseTable[i] = fseTableStream.read();
                        }
                        // Word 3 <accuracyLog><byteUsedByFSE>
                        //          1 byte          1 byte
                        litMeta = litMetaStream.read();
                        accuracyLog = (uint8_t)litMeta;
                        remBytesInFse = hufHeader - (litMeta >> 8);
                        // decode fse bitstream of huffman weights
                        fseDecodeHuffWeight<IN_BYTES>(litDecodeInStream, remBytesInFse, accHuff, bytesInAcc,
                                                      accuracyLog, litFseTable, hufWeights, fwCnt, huffDecoderTableLog);
                        remBytes = compressedSize - hufHeader - 1;
                        wCnt = fwCnt;
                    } else {
                        // 4-bit huffman weights
                        uint16_t hwCnt = hufHeader - 127;
                        uint32_t totalWeights = 0;
                        if (bytesInAcc == 0) {
                            accHuff.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                            bytesInAcc = c_inputByte;
                        }
                    huffman_decode_weights:
                        for (uint8_t i = 0; i < hwCnt; i += 2) {
#pragma HLS PIPELINE II = 1
                            if (bytesInAcc == 0) {
                                accHuff.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                                bytesInAcc = c_inputByte;
                            }
                            uint8_t w8t = accHuff;
                            accHuff >>= 8;
                            --bytesInAcc;
                            // get huffman weights
                            uint8_t hfw0 = (w8t >> 4);
                            uint8_t hfw1 = (w8t & 0x0F);
                            hufWeights[i] = hfw0;
                            hufWeights[i + 1] = hfw1;
                            // sum weights
                            totalWeights += ((((uint16_t)1 << hfw0) >> 1) + (((uint16_t)1 << hfw1) >> 1));
                        }
                        remBytes = compressedSize - (1 + (hwCnt - 1) / 2) - 1;
                        // last weight calculation
                        huffDecoderTableLog = 1 + (31 - __builtin_clz(totalWeights));
                        // add last weight
                        uint16_t lw = (1 << huffDecoderTableLog) - totalWeights;
                        hufWeights[hwCnt++] = 1 + (31 - __builtin_clz(lw));
                        wCnt = hwCnt;
                    }
                } else {
                    remBytes = compressedSize;
                }
                huffDecodeLiterals<IN_BYTES, OUT_BYTES, LMO_WIDTH, LOW_LATENCY>(
                    litDecodeInStream, quadStream, accHuff, bytesInAcc, remBytes, regeneratedSize, huffDecoderTableLog,
                    wCnt, hufWeights, literalStream);
            }
        }
    }
}