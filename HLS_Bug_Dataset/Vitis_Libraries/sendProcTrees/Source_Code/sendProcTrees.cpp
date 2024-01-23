void sendProcTrees(hls::stream<ap_uint<c_tgnSymbolBits> >& maxLdCodes,
                   hls::stream<ap_uint<c_tgnSymbolBits> >& maxBlCodes,
                   hls::stream<DSVectorStream_dt<Codeword, 1> >& Ldcodes,
                   hls::stream<DSVectorStream_dt<Codeword, 1> >& Blcodes,
                   hls::stream<bool>& isEOBlocks,
                   hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> >& hfcodeStream) {
    const uint8_t bitlen_vals[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
#pragma HLS ARRAY_PARTITION variable = bitlen_vals complete dim = 1

    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> outHufCode;
#pragma HLS AGGREGATE variable = outHufCode
send_tree_outer:
    while (isEOBlocks.read() == false) {
        Codeword zeroValue;
        zeroValue.bitlength = 0;
        zeroValue.codeword = 0;
        Codeword outCodes[c_litCodeCount + c_dstCodeCount + c_blnCodeCount + 2];
#pragma HLS AGGREGATE variable = outCodes

        ap_uint<c_tgnSymbolBits> maxCodesReg[3];

        const uint16_t offsets[3] = {0, c_litCodeCount + 1, (c_litCodeCount + c_dstCodeCount + 2)};
        // initialize all the memory
        maxCodesReg[0] = maxLdCodes.read();
        outHufCode.strobe = 1;
    read_litcodes_send:
        for (uint16_t i = 0; i < c_litCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }
        // last value write
        outCodes[c_litCodeCount] = zeroValue;
        maxCodesReg[1] = maxLdCodes.read();

    read_dstcodes_send:
        for (uint16_t i = 0; i < c_dstCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Ldcodes.read();
            outCodes[offsets[1] + i] = ldc.data[0];
            outHufCode.data[0].code = ldc.data[0].codeword;
            outHufCode.data[0].bitlen = ldc.data[0].bitlength;
            hfcodeStream << outHufCode;
        }

        outCodes[c_litCodeCount + c_dstCodeCount + 1] = zeroValue;
        maxCodesReg[2] = maxBlCodes.read();

    read_blcodes:
        for (uint16_t i = offsets[2]; i < offsets[2] + c_blnCodeCount; ++i) {
#pragma HLS PIPELINE II = 1
            auto ldc = Blcodes.read();
            outCodes[i] = ldc.data[0];
        }

        ap_uint<c_tgnSymbolBits> bl_mxc;
        bool mxb_continue = true;
    bltree_blen:
        for (bl_mxc = c_blnCodeCount - 1; (bl_mxc >= 3) && mxb_continue; --bl_mxc) {
#pragma HLS PIPELINE II = 1
            auto cIdx = offsets[2] + bitlen_vals[bl_mxc];
            mxb_continue = (outCodes[cIdx].bitlength == 0);
        }

        maxCodesReg[2] = bl_mxc + 1;

        // Code from Huffman Encoder
        //********************************************//
        // Start of block = 4 and len = 3
        // For dynamic tree
        outHufCode.strobe = 1;
        outHufCode.data[0].code = 4;
        outHufCode.data[0].bitlen = 3;
        hfcodeStream << outHufCode;
        // lcodes
        outHufCode.data[0].code = ((maxCodesReg[0] + 1) - 257);
        outHufCode.data[0].bitlen = 5;
        // dcodes
        outHufCode.data[1].code = ((maxCodesReg[1] + 1) - 1);
        outHufCode.data[1].bitlen = 5;
        // blcodes
        outHufCode.data[2].code = ((maxCodesReg[2] + 1) - 4);
        outHufCode.data[2].bitlen = 4;
        outHufCode.strobe = 3;
        hfcodeStream << outHufCode;

        ap_uint<c_tgnSymbolBits> bitIndex = offsets[2];
        outHufCode.strobe = 1;
    // Send BL length data
    send_bltree:
        for (ap_uint<c_tgnSymbolBits> rank = 0; rank < maxCodesReg[2] + 1; ++rank) {
#pragma HLS LOOP_TRIPCOUNT avg = 19 max = 19
#pragma HLS PIPELINE II = 1
            outHufCode.data[0].code = outCodes[bitIndex + bitlen_vals[rank]].bitlength;
            outHufCode.data[0].bitlen = 3;
            hfcodeStream << outHufCode;
        } // BL data copy loop

        Codeword temp;
#pragma HLS AGGREGATE variable = temp
    // Send Bitlengths for Literal and Distance Tree
    send_ld_trees_outer:
        for (int tree = 0; tree < 2; ++tree) {
            uint8_t prevlen = 0;                                 // Last emitted Length
            uint8_t curlen = 0;                                  // Length of Current Code
            uint8_t nextlen = outCodes[offsets[tree]].bitlength; // Length of next code
            uint8_t count = 0;
            int max_count = 7; // Max repeat count
            int min_count = 4; // Min repeat count

            if (nextlen == 0) {
                max_count = 138;
                min_count = 3;
            }

            uint16_t max_code = maxCodesReg[tree];
            temp = outCodes[bitIndex + c_reusePrevBlen];
            uint16_t reuse_prev_code = temp.codeword;
            uint8_t reuse_prev_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen];
            uint16_t reuse_zero_code = temp.codeword;
            uint8_t reuse_zero_len = temp.bitlength;
            temp = outCodes[bitIndex + c_reuseZeroBlen7];
            uint16_t reuse_zero7_code = temp.codeword;
            uint8_t reuse_zero7_len = temp.bitlength;

            uint8_t repCnt = 0;
            bool refCntFlag = true;
        send_ldtree_main:
            for (uint16_t n = 0; n <= max_code || repCnt > 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 30 max = 286
#pragma HLS PIPELINE II = 1
                if (repCnt != 0) {
                    uint8_t lclCnt = 0;
                    uint8_t tmp = repCnt;
                    bool cond = (repCnt <= 3);
                    repCnt = cond ? 0 : repCnt - 3;
                write_rep_codes_blens:
                    for (uint8_t k = 0; k < 3; ++k) {
#pragma HLS UNROLL
                        if (tmp > 0) {
                            outHufCode.data[k].code = temp.codeword;
                            outHufCode.data[k].bitlen = temp.bitlength;
                            --tmp;
                            ++lclCnt;
                        }
                    }
                    outHufCode.strobe = lclCnt;
                    hfcodeStream << outHufCode;
                    refCntFlag = (cond);
                } else {
                    refCntFlag = true;
                    curlen = nextlen;
                    // Length of next code
                    nextlen = outCodes[offsets[tree] + n + 1].bitlength;

                    if (++count < max_count && curlen == nextlen) {
                        refCntFlag = false;
                    } else if (count < min_count) {
                        temp = outCodes[bitIndex + curlen];
                        repCnt = count;
                        refCntFlag = false;
                    } else if (curlen != 0) {
                        uint8_t idx = 0;
                        if (curlen != prevlen) {
                            temp = outCodes[bitIndex + curlen];
                            outHufCode.data[0].code = temp.codeword;
                            outHufCode.data[0].bitlen = temp.bitlength;
                            idx = 1;
                            count--;
                        }
                        outHufCode.data[idx].code = reuse_prev_code;
                        outHufCode.data[idx].bitlen = reuse_prev_len;

                        outHufCode.data[idx + 1].code = count - 3;
                        outHufCode.data[idx + 1].bitlen = 2;

                        outHufCode.strobe = idx + 2;
                        hfcodeStream << outHufCode;
                    } else if (count <= 10) {
                        outHufCode.data[0].code = reuse_zero_code;
                        outHufCode.data[0].bitlen = reuse_zero_len;

                        outHufCode.data[1].code = count - 3;
                        outHufCode.data[1].bitlen = 3;
                        outHufCode.strobe = 2;
                        hfcodeStream << outHufCode;
                    } else {
                        outHufCode.data[0].code = reuse_zero7_code;
                        outHufCode.data[0].bitlen = reuse_zero7_len;

                        outHufCode.data[1].code = count - 11;
                        outHufCode.data[1].bitlen = 7;
                        outHufCode.strobe = 2;
                        hfcodeStream << outHufCode;
                    }
                    ++n;
                }
                if (refCntFlag) {
                    count = 0;
                    prevlen = curlen;
                    if (nextlen == 0) {
                        max_count = 138, min_count = 3;
                    } else if (curlen == nextlen) {
                        max_count = 6, min_count = 3;
                    } else {
                        max_count = 7, min_count = 4;
                    }
                }
            }
        }
        // ends huffman stream for each block, strobe eos not needed from this module
        outHufCode.strobe = 1;
        outHufCode.data[0].bitlen = 0;
        hfcodeStream << outHufCode;
    }
    // end of huffman tree data for all blocks
    outHufCode.strobe = 0;
    hfcodeStream << outHufCode;
}