void huffDecodeLiteralsSeq(hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inStream,
                           bool quadStream,
                           ap_uint<8 * PARALLEL_BYTE * 2> accHuff,
                           uint8_t bytesInAcc,
                           uint32_t remSize,
                           uint32_t regeneratedSize,
                           uint8_t accuracyLog,
                           uint16_t weightCnt,
                           uint8_t* weights,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& literalStream) {
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_BSWidth = 16;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const uint16_t c_accRegWidthx3 = c_streamWidth * 3;
    const uint16_t c_maxCodeLen = 11;
    // huffman lookup table
    HuffmanTable huffTable[2048];
#pragma HLS BIND_STORAGE variable = huffTable type = ram_t2p impl = bram

    ap_uint<c_BSWidth> bitStream[16 * 1024];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_t2p impl = bram
    uint16_t decSize[4];
    uint16_t cmpSize[4];
#pragma HLS ARRAY_PARTITION variable = cmpSize complete
#pragma HLS ARRAY_PARTITION variable = decSize complete
    uint8_t streamCnt = 1;

    // get stream sizes if 4 streams are present
    if (quadStream) {
        streamCnt = 4;
        // Jump table is 6 bytes long
        // read from input if needed
        if (bytesInAcc < PARALLEL_BYTE) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        // use 4 bytes
        // get decompressed size
        uint32_t dcmpSize = (regeneratedSize + 3) / 4;
        decSize[0] = decSize[1] = decSize[2] = dcmpSize;
        decSize[3] = regeneratedSize - (dcmpSize * 3);

        // get compressed size
        cmpSize[0] = accHuff;
        accHuff >>= 16;
        cmpSize[1] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 4;
        // read from input if needed
        if (bytesInAcc < 2) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        cmpSize[2] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 2;

        cmpSize[3] = remSize - (6 + cmpSize[0] + cmpSize[1] + cmpSize[2]);
    } else {
        decSize[0] = regeneratedSize;
        cmpSize[0] = remSize;
    }
    // generate huffman lookup table
    huffGenLookupTable<c_maxCodeLen>(weights, huffTable, accuracyLog, weightCnt);

    // decode bitstreams
    ap_uint<(8 * PARALLEL_BYTE)> outBuffer;
    uint8_t obbytes = 0;
    ap_uint<c_accRegWidth> bsbuff = accHuff;
    uint8_t bitsInAcc = bytesInAcc * 8;

    ap_uint<c_streamWidth> bsacc[c_maxCodeLen + 1];
#pragma HLS ARRAY_PARTITION variable = bsacc complete

decode_huff_bitstream_outer:
    for (uint8_t si = 0; si < streamCnt; ++si) {
        // copy data from bitstream to buffer
        uint32_t bsIdx = 0;
        uint8_t bitsWritten = c_BSWidth;
        const int bsPB = c_BSWidth / 8;
        int sIdx = 0;
    // write block data
    hufdlit_fill_bitstream:
        for (int i = 0; i < cmpSize[si]; i += bsPB) {
#pragma HLS PIPELINE II = 1
            if (i + bytesInAcc < cmpSize[si] && bytesInAcc < bsPB) {
                bsbuff.range(((bytesInAcc + PARALLEL_BYTE) * 8) - 1, bytesInAcc * 8) = inStream.read();
                bitsInAcc += c_streamWidth;
            }

            bitStream[bsIdx++] = bsbuff.range(c_BSWidth - 1, 0);

            if (i + bsPB > cmpSize[si]) bitsWritten = 8 * (cmpSize[si] - i);
            bsbuff >>= bitsWritten;
            bitsInAcc -= bitsWritten;
            bytesInAcc = bitsInAcc >> 3;
        }

        // generate decompressed bytes from huffman encoded stream
        ap_uint<c_streamWidth> acchbs = 0;
        uint8_t bitcnt = 0;
        int byteIndx = bsIdx - 1;
        uint32_t outBytes = 0;

        acchbs.range(c_BSWidth - 1, 0) = bitStream[byteIndx--];
        bitcnt = bitsWritten;
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (c_streamWidth - bitcnt);
        const int msbBitCnt = c_streamWidth - c_BSWidth;
        uint8_t shiftCnt = c_streamWidth - accuracyLog;
        uint8_t sym, blen = 0;
    // decode huffman bitstream
    huf_dec_bitstream:
        while (outBytes < decSize[si]) {
#pragma HLS PIPELINE II = 1
            // fill the acchbs in reverse
            if (bitcnt < 16 && byteIndx > -1) {
                uint32_t tmp = bitStream[byteIndx--];
                acchbs += tmp << (msbBitCnt - bitcnt);
                bitcnt += c_BSWidth;
            }

            uint16_t code = acchbs >> shiftCnt;

            sym = huffTable[code].symbol;
            blen = huffTable[code].bitlen;

        hfdbs_shift_acc:
            for (int s = 1; s < c_maxCodeLen + 1; ++s) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = c_maxCodeLen max = c_maxCodeLen
                bsacc[s] = acchbs << s;
            }
            bitcnt -= blen;
            acchbs = bsacc[blen];

            // write the literal to output stream
            outBuffer.range(((obbytes + 1) * 8) - 1, obbytes * 8) = sym;
            ++obbytes;
            if (obbytes == PARALLEL_BYTE) {
                literalStream << outBuffer;
                obbytes = 0;
                outBuffer = 0;
            }
            ++outBytes;
        }
    }
    if (obbytes > 0) {
        literalStream << outBuffer;
    }
}

// Content of called function
void huffGenLookupTable(uint8_t* weights, HuffmanTable* huffTable, uint8_t accuracyLog, uint16_t weightCnt) {
    // create huffman lookup table
    // regenerate huffman tree using literal bit-lengths
    typedef ap_uint<MAX_CODELEN + 1> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_CODELEN + 1];
    ap_uint<32> bitlen_histogram[MAX_CODELEN + 1];
    ap_uint<4> bitlens[256];
#pragma HLS ARRAY_PARTITION variable = first_codeword complete
#pragma HLS ARRAY_PARTITION variable = bitlen_histogram complete

    uint16_t codes[256];
// initialize first_codeword and bitlength histogram
hflkpt_init_blen_hist:
    for (uint8_t i = 0; i < MAX_CODELEN + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAX_CODELEN max = MAX_CODELEN
        bitlen_histogram[i] = 0;
    }
// read bit-lengths
hflkpt_fill_blen_hist:
    for (uint16_t i = 0; i < weightCnt; ++i) {
#pragma HLS PIPELINE II = 1
        // convert weight to bitlen
        uint8_t cblen = weights[i];
        bitlen_histogram[cblen]++;
        if (cblen > 0) cblen = (accuracyLog + 1 - cblen);
        bitlens[i] = cblen;
    }

    // generate first codes
    first_codeword[0] = bitlen_histogram[0];

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = 1; i < accuracyLog + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 8
        uint16_t cur = nextCode;
        nextCode += (bitlen_histogram[i] << (i - 1));
        first_codeword[i] = cur;
    }

hflkpt_codegen_outer:
    for (int i = 0; i < weightCnt; ++i) {
        uint32_t hfw = weights[i];
        const uint32_t len = (1 << hfw) >> 1;
        const auto fcw = first_codeword[hfw];
    hflkpt_codegen:
        for (uint16_t u = fcw; u < fcw + len; ++u) {
#pragma HLS PIPELINE II = 1
            huffTable[u].symbol = i;
            huffTable[u].bitlen = bitlens[i];
        }
        first_codeword[hfw] = fcw + len;
    }
}