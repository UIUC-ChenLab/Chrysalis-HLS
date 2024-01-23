void decodeSequence(hls::stream<bool>& seqValidStream,
                    hls::stream<uint32_t>& seqMetaStream,
                    hls::stream<uint32_t>& fseTableStream,
                    hls::stream<ap_uint<IN_BYTES * 8> >& seqDecodeInStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& litLenStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& offsetStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& matLenStream,
                    hls::stream<bool>& litlenValidStream,
                    hls::stream<bool>& newBlockFlagStream) {
    // decode sequences and output literal lengths, offsets and match lengths
    const uint8_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    uint32_t fseTableLL[512];
#pragma HLS BIND_STORAGE variable = fseTableLL type = ram_t2p impl = bram
    uint32_t fseTableOF[256];
#pragma HLS BIND_STORAGE variable = fseTableOF type = ram_t2p impl = bram
    uint32_t fseTableML[512];
#pragma HLS BIND_STORAGE variable = fseTableML type = ram_t2p impl = bram
    ap_uint<LMO_WIDTH> prevOffsets[3] = {1, 4, 8}; // list of previous 3 offsets
#pragma HLS ARRAY_PARTITION variable = prevOffsets complete

decodeSequence_main_loop:
    while (seqValidStream.read()) {
        uint32_t seqMeta = seqMetaStream.read();
        xfBlockType_t blockType = (xfBlockType_t)(seqMeta & 0x000000FF);
        uint32_t blockSize = seqMeta >> 8;
        newBlockFlagStream << 1;
        if (blockType == RLE_BLOCK) {
            litlenValidStream << 1;
            litLenStream << 1;
            offsetStream << 1;
            matLenStream << blockSize - 1;
        } else if (blockType == RAW_BLOCK) {
            litlenValidStream << 1;
            litLenStream << blockSize;
            offsetStream << 0;
            matLenStream << 0;
        } else {
            // write sequence meta data
            // Word 0 <blockMeta> ***already decoded
            // Word 1 <decode_flag> 1 if further decoding needed, else 0
            // Word 2 <literalCount>
            // Word 3 <symbolCompressionMode><remBlockSize>
            //              1 byte              3 bytes
            // Word 4 <seqCnt>
            // Word 5 <AccuracyLogs> --> <litlen><offset><matlen> 1 byte each lower-higher
            // decode fse bitstream
            uint8_t decode_flag = seqMetaStream.read();
            uint32_t litCount = seqMetaStream.read();
            if (decode_flag) {
            // read all fse tables
            seqd_read_fse_tables_outer:
                for (int fti = 0; fti < 3; ++fti) {
                    uint16_t fseVCnt = fseTableStream.read();
                    if (fseVCnt != 0xFFFFFFFF) {
                    seqd_read_fse_tables:
                        for (uint16_t i = 0; i < fseVCnt; ++i) {
#pragma HLS PIPELINE II = 1
                            auto tmpv = fseTableStream.read();
                            if (fti == 0) {
                                fseTableLL[i] = tmpv;
                            } else if (fti == 1) {
                                fseTableOF[i] = tmpv;
                            } else {
                                fseTableML[i] = tmpv;
                            }
                        }
                    }
                }

                uint32_t smbuf = seqMetaStream.read();
                uint32_t remBlockSize = smbuf >> 8;
                uint32_t seqCnt = seqMetaStream.read();
                uint32_t aclbuf = seqMetaStream.read(); // accuracy logs
                uint8_t accuracyLog[3] = {(uint8_t)aclbuf, (uint8_t)(aclbuf >> 8), (uint8_t)(aclbuf >> 16)};
                // Decode the FSE compressed bitstream
                ap_uint<64> acw = 0;
                fseDecode<IN_BYTES, BLOCK_SIZE_KB, LMO_WIDTH, LOW_LATENCY>(
                    acw, 0, seqDecodeInStream, fseTableLL, fseTableOF, fseTableML, seqCnt, litCount, remBlockSize,
                    accuracyLog, prevOffsets, litLenStream, offsetStream, matLenStream, litlenValidStream);
            } else {
                // zero sequence count condition
                litlenValidStream << 1;
                litLenStream << litCount;
                offsetStream << 0;
                matLenStream << 0;
                // update previous offsets
                prevOffsets[2] = prevOffsets[1];
                prevOffsets[1] = prevOffsets[0];
                prevOffsets[0] = 0;
            }
        }
        // clear LZ literal buffer
        litlenValidStream << 0;
    }
    // end of file
    newBlockFlagStream << 0;
    matLenStream << 0;
    offsetStream << 0;
}