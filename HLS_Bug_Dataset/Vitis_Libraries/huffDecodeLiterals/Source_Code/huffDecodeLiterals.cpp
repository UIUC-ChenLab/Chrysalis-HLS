void huffDecodeLiterals(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                        bool quadStream,
                        ap_uint<IN_BYTES * 8 * 2> accHuff,
                        uint8_t bytesInAcc,
                        ap_uint<LMO_WIDTH> remSize,
                        ap_uint<LMO_WIDTH> regeneratedSize,
                        uint8_t accuracyLog,
                        uint16_t weightCnt,
                        uint8_t* weights,
                        hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    const uint8_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_maxCodeLen = 11;
    uint8_t streamCnt = 1;
    uint16_t decSize[4];
    uint16_t decSize1[4];
    uint16_t cmpSize[4];
#pragma HLS ARRAY_PARTITION variable = decSize complete
#pragma HLS ARRAY_PARTITION variable = decSize1 complete
#pragma HLS ARRAY_PARTITION variable = cmpSize complete
    // get stream sizes if 4 streams are present
    if (quadStream) {
        streamCnt = 4;
        // Jump table is 6 bytes long
        // read from input if needed
        if (bytesInAcc < c_inputByte) {
            accHuff.range(((c_inputByte + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += c_inputByte;
        }
        // use 4 bytes
        // get decompressed size
        ap_uint<LMO_WIDTH> dcmpSize = (regeneratedSize + 3) / 4;
        decSize[0] = decSize[1] = decSize[2] = dcmpSize;
        decSize1[0] = decSize1[1] = decSize1[2] = dcmpSize;
        decSize[3] = regeneratedSize - (dcmpSize * 3);
        decSize1[3] = decSize[3];

        // get compressed size
        cmpSize[0] = accHuff;
        accHuff >>= 16;
        cmpSize[1] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 4;
        // read from input if needed
        if (bytesInAcc < 2) {
            accHuff.range(((c_inputByte + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += c_inputByte;
        }
        cmpSize[2] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 2;

        remSize -= 6;
        cmpSize[3] = remSize - (cmpSize[0] + cmpSize[1] + cmpSize[2]);
    } else {
        decSize[0] = regeneratedSize;
        decSize1[0] = regeneratedSize;
        cmpSize[0] = remSize;
    }
    // parallel huffman decoding
    if (LOW_LATENCY) {
        huffDecodeLitInternalLL<IN_BYTES, OUT_BYTES, c_maxCodeLen>(inStream, accHuff, bytesInAcc, cmpSize, decSize,
                                                                   decSize1, accuracyLog, streamCnt, weightCnt, weights,
                                                                   literalStream);
    } else {
        huffDecodeLitInternal<IN_BYTES, OUT_BYTES, c_maxCodeLen>(
            inStream, accHuff, bytesInAcc, cmpSize, decSize, accuracyLog, streamCnt, weightCnt, weights, literalStream);
    }
}

// Content of called function
void huffDecodeLitInternal(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                           ap_uint<IN_BYTES * 8 * 2> accHuff,
                           uint8_t bytesInAcc,
                           uint16_t* cmpSize,
                           uint16_t* decSize,
                           uint8_t accuracyLog,
                           uint8_t streamCnt,
                           uint16_t weightCnt,
                           uint8_t* weights,
                           hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    const uint8_t c_BSWidth = 16;
    const uint8_t c_symWidth = 9;
    // internal streams
    hls::stream<IntVectorStream_dt<c_BSWidth, 1> > huffBitStream("huffBitStream");
    hls::stream<ap_uint<8> > validBitCntStream("validBitCntStream");
    hls::stream<ap_uint<c_symWidth> > byteLiteralStream("byteLiteralStream");
#pragma HLS STREAM variable = huffBitStream depth = 32
#pragma HLS STREAM variable = validBitCntStream depth = 8
#pragma HLS STREAM variable = byteLiteralStream depth = 8

#pragma HLS DATAFLOW

    hfdDataFeader<IN_BYTES, c_BSWidth>(inStream, streamCnt, cmpSize, accHuff, bytesInAcc, huffBitStream,
                                       validBitCntStream);
    hfdGetCodesStreamLiterals<MAX_CODELEN, c_BSWidth>(decSize, accuracyLog, streamCnt, weightCnt, weights,
                                                      huffBitStream, validBitCntStream, byteLiteralStream);
    writeAccLiteralData<OUT_BYTES>(byteLiteralStream, literalStream);
}

// Content of called function
void hfdDataFeader(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                   uint8_t streamCnt,
                   uint16_t* cmpSize,
                   ap_uint<IN_BYTES * 8 * 2> accHuff,
                   uint8_t bytesInAcc,
                   hls::stream<IntVectorStream_dt<BS_WIDTH, 1> >& huffBitStream,
                   hls::stream<ap_uint<8> >& validBitCntStream) {
    const uint8_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_BSWidth = BS_WIDTH;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const int c_bsPB = c_BSWidth / 8;
    const int c_bsUpperLim = (((32 / c_bsPB) / 2) * 1024);

    ap_uint<c_BSWidth> bitStream[c_bsUpperLim];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_t2p impl = bram

    // internal registers
    ap_uint<c_accRegWidth> bsbuff = accHuff; // must not contain more than 3 bytes
    uint8_t bitsInAcc = bytesInAcc * 8;

    int wIdx = 0;
    int rIdx = 0;
    int fmInc = 1; // can be +1 or -1
    int smInc = 1;
    uint8_t bitsWritten = c_BSWidth;
    int streamRBgn[4];     // starting index for BRAM read
    int streamRLim[4 + 1]; // point/index till which the BRAM can be read, 1 extra buffer entry
#pragma HLS ARRAY_PARTITION variable = streamRBgn complete
#pragma HLS ARRAY_PARTITION variable = streamRLim complete
    uint8_t wsi = 0, rsi = 0;
    int inIdx = 0;
    // modes
    bool fetchMode = 1;
    bool streamMode = 0;
    bool done = 0;
    // initialize
    streamRLim[wsi] = 0; // initial stream will stream from higher to lower address
hfdl_dataStreamer:
    while (!done) {
#pragma HLS PIPELINE II = 1
        // stream data, bitStream buffer width is equal to inStream width for simplicity
        if (fetchMode) {
            // fill bitstream in direction specified by increment variable
            if (inIdx + bytesInAcc < cmpSize[wsi] && bytesInAcc < c_bsPB) {
                bsbuff.range(bitsInAcc + c_streamWidth - 1, bitsInAcc) = inStream.read();
                bitsInAcc += c_streamWidth;
            }
            bitStream[wIdx] = bsbuff.range(c_BSWidth - 1, 0);
            if (inIdx + c_bsPB >= cmpSize[wsi]) {
                auto bw = 8 * (cmpSize[wsi] - inIdx);
                bitsWritten = (bw == 0) ? bitsWritten : bw;
                validBitCntStream << bitsWritten;
                bsbuff >>= bitsWritten;
                bitsInAcc -= bitsWritten;
                bytesInAcc = bitsInAcc >> 3;

                // update fetch mode state
                if (streamMode == 0) {
                    streamMode = 1;
                    rIdx = wIdx;
                }
                inIdx = 0;            // just an index, not directional
                fmInc = (~fmInc) + 1; // flip 1 and -1
                streamRBgn[wsi] = wIdx;
                ++wsi;
                if (wsi & 1) {
                    streamRLim[wsi] = c_bsUpperLim - 1;
                    wIdx = c_bsUpperLim - 1;
                } else {
                    streamRLim[wsi] = 0;
                    wIdx = 0;
                }
                // post increment checks
                if ((wsi == streamCnt) || (wsi - rsi > 1)) fetchMode = 0;
                // reset default value
                bitsWritten = c_BSWidth;
                continue;
            }
            bsbuff >>= bitsWritten;
            bitsInAcc -= bitsWritten;
            bytesInAcc = bitsInAcc >> 3;

            inIdx += c_bsPB;
            wIdx += fmInc;
        }
        if (streamMode) {
            // write data to output stream
            uint32_t tmp = bitStream[rIdx];
            // output register
            IntVectorStream_dt<BS_WIDTH, 1> outValue;
            outValue.data[0] = tmp;
            // update stream mode state
            if (rIdx == streamRLim[rsi]) {
                outValue.strobe = 0;
                ++rsi;
                rIdx = streamRBgn[rsi];
                smInc = (~smInc) + 1; // flip 1 and -1
                // no need to check if fetchMode == 0
                if (wsi < streamCnt) fetchMode = 1;
                // either previous streamMode ended quicker than next fetchMode or streamCnt reached
                if (wsi == rsi) streamMode = 0;
            } else {
                outValue.strobe = 1;
                rIdx -= smInc;
            }
            huffBitStream << outValue;
        }
        // end condition
        if (!(fetchMode | streamMode)) done = 1;
    }
}

// Content of called function
void hfdGetCodesStreamLiterals(uint16_t* decSize,
                               uint8_t accuracyLog,
                               uint8_t streamCnt,
                               uint16_t weightCnt,
                               uint8_t* weights,
                               hls::stream<IntVectorStream_dt<BS_WIDTH, 1> >& huffBitStream,
                               hls::stream<ap_uint<8> >& validBitCntStream,
                               hls::stream<ap_uint<9> >& literalStream) {
    const uint16_t c_HBFSize = 32;
    const uint16_t c_BSWidth = BS_WIDTH;
    const int c_bsPB = c_BSWidth / 8;

    ap_uint<11> validCodeOffset;
    ap_uint<4> bitLen;
    ap_uint<8> symbol[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = symbol complete dim = 0

    // New huffman code
    ap_uint<MAX_CODELEN + 1> codeOffsets[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = codeOffsets dim = 1 complete

    ap_uint<8> bl1Code[2];
    ap_uint<8> bl2Code[4];
    ap_uint<8> bl3Code[8];
    ap_uint<8> bl4Code[16];
    ap_uint<8> bl5Code[32];
    ap_uint<8> bl6Code[64];
    ap_uint<8> bl7Code[128];
    ap_uint<8> bl8Code[256];
    ap_uint<8> bl9Code[256];
    ap_uint<8> bl10Code[256];
    ap_uint<8> bl11Code[256];
#pragma HLS BIND_STORAGE variable = bl1Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl2Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl3Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl4Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl5Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl6Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl7Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl8Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl9Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl10Code type = ram_1p impl = lutram
#pragma HLS BIND_STORAGE variable = bl11Code type = ram_1p impl = lutram

    // generate codes
    code_generator<MAX_CODELEN>(weights, codeOffsets, bl1Code, bl2Code, bl3Code, bl4Code, bl5Code, bl6Code, bl7Code,
                                bl8Code, bl9Code, bl10Code, bl11Code, accuracyLog, weightCnt);

    ap_uint<9> outBuffer;
    uint8_t obbytes = 0;
decode_huff_bitstream_outer:
    for (uint8_t si = 0; si < streamCnt; ++si) {
        // generate decompressed bytes from huffman encoded stream
        ap_uint<c_HBFSize> acchbs = 0;
        ap_uint<8> bitcnt = 0;
        uint32_t outBytes = 0;

        bitcnt = validBitCntStream.read();
        // input register
        auto inValue = huffBitStream.read();
        acchbs.range(c_BSWidth - 1, 0) = inValue.data[0];
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (c_HBFSize - bitcnt);
        uint16_t byteIndx = c_bsPB; // just to verify with the compressed size
        const int msbBitCnt = c_HBFSize - c_BSWidth;

        if ((bitcnt.range(4, 4) != 1) && (inValue.strobe == 1)) {
            inValue = huffBitStream.read();
            uint32_t tmp = inValue.data[0];
            acchbs += tmp << (msbBitCnt - bitcnt);
            bitcnt += c_BSWidth;
            byteIndx += c_bsPB;
        }

    // decode huffman bitstreams
    huf_dec_bitstream:
        for (uint32_t outBytes = 0; outBytes < decSize[si]; outBytes += 1) {
#pragma HLS PIPELINE II = 1

            for (uint8_t i = 0; i < MAX_CODELEN; ++i) {
#pragma HLS UNROLL
                validCodeOffset.range(i, i) = (acchbs.range(31, 31 - i) >= codeOffsets[i]) ? 1 : 0;
            }

            bitLen = 1 + ap_uint<6>(__builtin_ctz((unsigned int)(validCodeOffset)));

            symbol[1] = bl1Code[acchbs.range(31, 31)];
            symbol[2] = bl2Code[acchbs.range(31, 30)];
            symbol[3] = bl3Code[acchbs.range(31, 29)];
            symbol[4] = bl4Code[acchbs.range(31, 28)];
            symbol[5] = bl5Code[acchbs.range(31, 27)];
            symbol[6] = bl6Code[acchbs.range(31, 26)];
            symbol[7] = bl7Code[acchbs.range(31, 25)];
            symbol[8] = bl8Code[acchbs.range(31, 24)];
            symbol[9] = bl9Code[ap_uint<8>(acchbs.range(31, 23))];
            symbol[10] = bl10Code[ap_uint<8>(acchbs.range(31, 22))];
            symbol[11] = bl11Code[ap_uint<8>(acchbs.range(31, 21))];

            bitcnt -= bitLen;
            acchbs <<= bitLen;

            // write the literal to output stream
            outBuffer.range(7, 0) = symbol[bitLen];
            outBuffer.range(8, 8) = true;
            literalStream << outBuffer;

            if ((bitcnt.range(4, 4) != 1) && (inValue.strobe == 1)) {
                inValue = huffBitStream.read();
                uint32_t tmp = inValue.data[0];
                acchbs += tmp << (msbBitCnt - bitcnt);
                bitcnt += c_BSWidth;
                byteIndx += c_bsPB;
            }
        }
    }

    outBuffer = 0;
    literalStream << outBuffer;
}

// Content of called function
void code_generator(uint8_t* weights,
                    ap_uint<MAX_CODELEN + 1>* codeOffsets,
                    ap_uint<8>* bl1Codes,
                    ap_uint<8>* bl2Codes,
                    ap_uint<8>* bl3Codes,
                    ap_uint<8>* bl4Codes,
                    ap_uint<8>* bl5Codes,
                    ap_uint<8>* bl6Codes,
                    ap_uint<8>* bl7Codes,
                    ap_uint<8>* bl8Codes,
                    ap_uint<8>* bl9Codes,
                    ap_uint<8>* bl10Codes,
                    ap_uint<8>* bl11Codes,
                    uint8_t accuracyLog,
                    uint16_t weightCnt) {
    // regenerate huffman tree using literal bit-lengths
    typedef ap_uint<MAX_CODELEN + 1> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_CODELEN + 1];
    ap_uint<8> bitlen_histogram[MAX_CODELEN + 1];
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
        if (cblen > 0) cblen = (accuracyLog + 1 - cblen);
        bitlens[i] = cblen;
        bitlen_histogram[cblen]++;
    }

    // generate first codes
    first_codeword[0] = 0;
    codeOffsets[0] = 0;

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (int8_t i = accuracyLog - 1; i >= 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 11
        uint16_t cur = nextCode;
        nextCode += (bitlen_histogram[i + 1]);
        nextCode >>= 1;
        first_codeword[i] = cur;
        codeOffsets[i] = cur;
    }

    uint16_t blen = 0;
CodeGen:
    for (uint16_t i = 0; i < weightCnt; i++) {
#pragma HLS PIPELINE II = 1
        blen = bitlens[i];
        if (blen != 0) {
            switch (blen) {
                case 1:
                    bl1Codes[first_codeword[0]] = i;
                    break;
                case 2:
                    bl2Codes[first_codeword[1]] = i;
                    break;
                case 3:
                    bl3Codes[first_codeword[2]] = i;
                    break;
                case 4:
                    bl4Codes[first_codeword[3]] = i;
                    break;
                case 5:
                    bl5Codes[first_codeword[4]] = i;
                    break;
                case 6:
                    bl6Codes[first_codeword[5]] = i;
                    break;
                case 7:
                    bl7Codes[first_codeword[6]] = i;
                    break;
                case 8:
                    bl8Codes[first_codeword[7]] = i;
                    break;
                case 9:
                    bl9Codes[ap_uint<8>(first_codeword[8])] = i;
                    break;
                case 10:
                    bl10Codes[ap_uint<8>(first_codeword[9])] = i;
                    break;
                case 11:
                    bl11Codes[ap_uint<8>(first_codeword[10])] = i;
                    break;
                default:
                    assert(0);
                    break;
            }
            first_codeword[blen - 1]++;
        }
    }
}

// Content of called function
void writeAccLiteralData(hls::stream<ap_uint<9> >& byteStream,
                         hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    ap_uint<OUT_BYTES * 8> outBuffer;
    IntVectorStream_dt<OUT_BYTES * 8, 1> outValue;
    ap_uint<4> writeIdx = 0;

huffLitUpsizer:
    for (ap_uint<9> inData = byteStream.read(); inData.range(8, 8) == 1; inData = byteStream.read()) {
#pragma HLS PIPELINE II = 1
        outBuffer.range((writeIdx + 1) * 8 - 1, writeIdx * 8) = inData.range(7, 0);
        writeIdx++;
        if (writeIdx == OUT_BYTES) {
            outValue.data[0] = outBuffer;
            outValue.strobe = 1;
            literalStream << outValue;
            writeIdx = 0;
        }
    }

    if (writeIdx) {
        outValue.data[0] = outBuffer;
        outValue.strobe = 1;
        literalStream << outValue;
    }
    outValue.strobe = 0;
    literalStream << outValue;
}

// Content of called function
void huffDecodeLitInternalLL(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                             ap_uint<IN_BYTES * 8 * 2> accHuff,
                             uint8_t bytesInAcc,
                             uint16_t* cmpSize,
                             uint16_t* decSize,
                             uint16_t* decSize1,
                             uint8_t accuracyLog,
                             uint8_t streamCnt,
                             uint16_t weightCnt,
                             uint8_t* weights,
                             hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    const uint8_t c_BSWidth = 24;
    const uint8_t c_symWidth = 17;
    // internal streams
    hls::stream<IntVectorStream_dt<c_BSWidth, 1> > huffBitStream("huffBitStream");
    hls::stream<ap_uint<8> > validBitCntStream("validBitCntStream");
    hls::stream<ap_uint<c_symWidth> > byteLiteralStream("byteLiteralStream");
#pragma HLS STREAM variable = huffBitStream depth = 32
#pragma HLS STREAM variable = validBitCntStream depth = 8
#pragma HLS STREAM variable = byteLiteralStream depth = 8

#pragma HLS DATAFLOW
    hfdDataFeader<IN_BYTES, c_BSWidth>(inStream, streamCnt, cmpSize, accHuff, bytesInAcc, huffBitStream,
                                       validBitCntStream);
    hfdGetCodesStreamLiteralsLL<MAX_CODELEN, c_BSWidth>(decSize, accuracyLog, streamCnt, weightCnt, weights,
                                                        huffBitStream, validBitCntStream, byteLiteralStream);
    writeAccLiteralDataLL<OUT_BYTES>(byteLiteralStream, literalStream, decSize1);
}

// Content of called function
void hfdGetCodesStreamLiteralsLL(uint16_t* decSize,
                                 uint8_t accuracyLog,
                                 uint8_t streamCnt,
                                 uint16_t weightCnt,
                                 uint8_t* weights,
                                 hls::stream<IntVectorStream_dt<BS_WIDTH, 1> >& huffBitStream,
                                 hls::stream<ap_uint<8> >& validBitCntStream,
                                 hls::stream<ap_uint<17> >& literalStream) {
    const uint16_t c_HBFSize = 48;
    const uint16_t c_BSWidth = BS_WIDTH;
    const int c_bsPB = c_BSWidth / 8;

    ap_uint<11> validCodeOffset[MAX_CODELEN + 1];
    ap_uint<4> bitLen[MAX_CODELEN + 1];
    ap_uint<8> symbol[2][MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = validCodeOffset complete dim = 0
#pragma HLS ARRAY_PARTITION variable = bitLen complete dim = 0
#pragma HLS ARRAY_PARTITION variable = symbol complete dim = 0

    // New huffman code
    ap_uint<MAX_CODELEN + 1> codeOffsets[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = codeOffsets dim = 1 complete

    ap_uint<8> bl1Code[2];
    ap_uint<8> bl2Code[4];
    ap_uint<8> bl3Code[8];
    ap_uint<8> bl4Code[16];
    ap_uint<8> bl5Code[32];
    ap_uint<8> bl6Code[64];
    ap_uint<8> bl7Code[128];
    ap_uint<8> bl8Code[256];
    ap_uint<8> bl9Code[256];
    ap_uint<8> bl10Code[256];
    ap_uint<8> bl11Code[256];
#pragma HLS BIND_STORAGE variable = bl1Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl2Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl3Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl4Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl5Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl6Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl7Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl8Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl9Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl10Code type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = bl11Code type = ram_2p impl = lutram

    // generate codes
    code_generator<MAX_CODELEN>(weights, codeOffsets, bl1Code, bl2Code, bl3Code, bl4Code, bl5Code, bl6Code, bl7Code,
                                bl8Code, bl9Code, bl10Code, bl11Code, accuracyLog, weightCnt);

    ap_uint<17> outBuffer;
decode_huff_bitstream_outer:
    for (ap_uint<3> si = 0; si < streamCnt; ++si) {
        // generate decompressed bytes from huffman encoded stream
        ap_uint<c_HBFSize> acchbs = 0;
        ap_uint<7> bitcnt = 0;

        bitcnt = validBitCntStream.read();
        // input register
        auto inValue = huffBitStream.read();
        acchbs.range(c_BSWidth - 1, 0) = inValue.data[0];
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (c_HBFSize - bitcnt);
        const int c_msbBitCnt = c_HBFSize - c_BSWidth;

        ap_uint<11> parallelAcc[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = parallelAcc complete
        ap_uint<8> tmpBitLen = bitcnt;
        bool readUpdFlag = ((tmpBitLen < 22) && (inValue.strobe == 1));
        uint8_t shiftCnt = c_msbBitCnt - tmpBitLen;

        ap_uint<c_HBFSize> tmp = 0;
        if (readUpdFlag) {
            inValue = huffBitStream.read();
            tmp = inValue.data[0];
            bitcnt += c_BSWidth;
        }
        ap_uint<c_HBFSize> acchbs1 = acchbs;
        ap_uint<c_HBFSize> acchbs2 = tmp << shiftCnt;
        acchbs = acchbs1 | acchbs2;

        // pre-read one data
        inValue = huffBitStream.read();

    // decode huffman bitstreams
    huf_dec_bitstream:
        for (uint32_t outBytes = 0; outBytes < decSize[si]; outBytes += 2) {
#pragma HLS PIPELINE II = 1

            for (ap_uint<4> i = 0; i < MAX_CODELEN + 1; ++i) {
#pragma HLS UNROLL
                parallelAcc[i] = acchbs.range(c_HBFSize - 1 - i, c_HBFSize - MAX_CODELEN - i);
                for (ap_uint<4> j = 0; j < MAX_CODELEN; ++j) {
#pragma HLS UNROLL
                    validCodeOffset[i].range(j, j) = (parallelAcc[i].range(10, 10 - j) >= codeOffsets[j]) ? 1 : 0;
                }
                bitLen[i] = 1 + ap_uint<6>(__builtin_ctz((unsigned int)(validCodeOffset[i])));
            }

            ap_uint<5> totalBitLen = bitLen[0] + bitLen[bitLen[0]];
            ap_uint<7> tmpBitLen = bitcnt - totalBitLen;
            ap_uint<5> shiftCnt = c_msbBitCnt - tmpBitLen;
            if (tmpBitLen < 22) {
                tmp = inValue.data[0];
                bitcnt = c_BSWidth + tmpBitLen;
            } else {
                tmp = 0;
                bitcnt = tmpBitLen;
            }
            ap_uint<c_HBFSize> acchbs1 = acchbs << totalBitLen;
            ap_uint<c_HBFSize> acchbs2 = tmp << shiftCnt;
            acchbs = acchbs1 | acchbs2;

            bool readUpdFlag = ((tmpBitLen < 22) && (inValue.strobe == 1));
            if (readUpdFlag) {
                inValue = huffBitStream.read();
            }
            symbol[0][1] = bl1Code[parallelAcc[0].range(10, 10)];
            symbol[0][2] = bl2Code[parallelAcc[0].range(10, 9)];
            symbol[0][3] = bl3Code[parallelAcc[0].range(10, 8)];
            symbol[0][4] = bl4Code[parallelAcc[0].range(10, 7)];
            symbol[0][5] = bl5Code[parallelAcc[0].range(10, 6)];
            symbol[0][6] = bl6Code[parallelAcc[0].range(10, 5)];
            symbol[0][7] = bl7Code[parallelAcc[0].range(10, 4)];
            symbol[0][8] = bl8Code[parallelAcc[0].range(10, 3)];
            symbol[0][9] = bl9Code[ap_uint<8>(parallelAcc[0].range(10, 2))];
            symbol[0][10] = bl10Code[ap_uint<8>(parallelAcc[0].range(10, 1))];
            symbol[0][11] = bl11Code[ap_uint<8>(parallelAcc[0].range(10, 0))];

            symbol[1][1] = bl1Code[parallelAcc[bitLen[0]].range(10, 10)];
            symbol[1][2] = bl2Code[parallelAcc[bitLen[0]].range(10, 9)];
            symbol[1][3] = bl3Code[parallelAcc[bitLen[0]].range(10, 8)];
            symbol[1][4] = bl4Code[parallelAcc[bitLen[0]].range(10, 7)];
            symbol[1][5] = bl5Code[parallelAcc[bitLen[0]].range(10, 6)];
            symbol[1][6] = bl6Code[parallelAcc[bitLen[0]].range(10, 5)];
            symbol[1][7] = bl7Code[parallelAcc[bitLen[0]].range(10, 4)];
            symbol[1][8] = bl8Code[parallelAcc[bitLen[0]].range(10, 3)];
            symbol[1][9] = bl9Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 2))];
            symbol[1][10] = bl10Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 1))];
            symbol[1][11] = bl11Code[ap_uint<8>(parallelAcc[bitLen[0]].range(10, 0))];

            outBuffer.range(7, 0) = symbol[0][bitLen[0]];
            outBuffer.range(15, 8) = symbol[1][bitLen[bitLen[0]]];
            outBuffer.range(16, 16) = true;

            // write the literal to output stream
            literalStream << outBuffer;
        }
    }

    outBuffer = 0;
    literalStream << outBuffer;
}

// Content of called function
void writeAccLiteralDataLL(hls::stream<ap_uint<17> >& byteStream,
                           hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream,
                           uint16_t* decSize) {
    const uint8_t c_outputByte = OUT_BYTES;
    const int c_streamWidth = c_outputByte * 8;
    ap_uint<c_streamWidth + 16> outBuffer;
    ap_uint<4> writeIdx = 0;
    uint8_t i = 0;
    uint32_t outBytes = 0;
    IntVectorStream_dt<OUT_BYTES * 8, 1> outValue;

writeAccLiterals:
    for (ap_uint<17> inData = byteStream.read(); inData.range(16, 16) == 1; inData = byteStream.read()) {
#pragma HLS PIPELINE II = 1
        outBytes += 2;
        outBuffer.range((writeIdx + 1) * 8 - 1, writeIdx * 8) = inData.range(7, 0);
        outBuffer.range((writeIdx + 2) * 8 - 1, (writeIdx + 1) * 8) = inData.range(15, 8);

        if (outBytes <= decSize[i]) {
            writeIdx += 2;
        } else {
            writeIdx += 1;
        }

        if (outBytes >= decSize[i]) {
            i += 1;
            outBytes = 0;
        }

        if (writeIdx >= c_outputByte) {
            outValue.data[0] = outBuffer;
            outValue.strobe = 1;
            literalStream << outValue;
            outBuffer >>= c_streamWidth;
            writeIdx -= c_outputByte;
        }
    }

    if (writeIdx) {
        outValue.data[0] = outBuffer;
        outValue.strobe = 1;
        literalStream << outValue;
    }
    outValue.strobe = 0;
    literalStream << outValue;
}