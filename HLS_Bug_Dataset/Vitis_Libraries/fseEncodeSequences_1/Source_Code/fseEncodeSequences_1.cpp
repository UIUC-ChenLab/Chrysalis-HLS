void fseEncodeSequences(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                        hls::stream<IntVectorStream_dt<36, 1> >& fseSeqTableStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& encodedOutStream,
                        hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& seqEncSizeStream) {
    // internal streams
    hls::stream<DSVectorStream_dt<Sequence_dt<6, 6, 6>, 1> > seqCodeStream("seqCodeStream");
    hls::stream<bool> noSeqFlagStream("noSeqFlagStream");
    hls::stream<ap_uint<33> > extCodewordStream("extCodewordStream");
    hls::stream<ap_uint<8> > extBitlenStream("extBitlenStream");
    hls::stream<IntVectorStream_dt<28, 1> > seqFseWordStream("seqFseWordStream");
    hls::stream<ap_uint<5> > wordBitlenStream("wordBitlenStream");

#pragma HLS STREAM variable = seqCodeStream depth = 4
#pragma HLS STREAM variable = noSeqFlagStream depth = 4
#pragma HLS STREAM variable = extCodewordStream depth = 16
#pragma HLS STREAM variable = extBitlenStream depth = 16
#pragma HLS STREAM variable = seqFseWordStream depth = 4
#pragma HLS STREAM variable = wordBitlenStream depth = 4

#pragma HLS dataflow

    fseGetSeqCodes<MAX_FREQ_DWIDTH>(inSeqStream, seqCodeStream, noSeqFlagStream, extCodewordStream, extBitlenStream);

    fseEncodeSeqCodes(fseSeqTableStream, seqCodeStream, noSeqFlagStream, seqFseWordStream, wordBitlenStream);

    seqFseBitPacker<MAX_FREQ_DWIDTH>(seqFseWordStream, wordBitlenStream, extCodewordStream, extBitlenStream,
                                     encodedOutStream, seqEncSizeStream);
}

// Content of called function
void fseEncodeSeqCodes(hls::stream<IntVectorStream_dt<36, 1> >& fseSeqTableStream,
                       hls::stream<DSVectorStream_dt<Sequence_dt<6, 6, 6>, 1> >& seqCodeStream,
                       hls::stream<bool>& noSeqFlagStream,
                       hls::stream<IntVectorStream_dt<28, 1> >& seqFseWordStream,
                       hls::stream<ap_uint<5> >& wordBitlenStream) {
    // Encode sequence codes
    // Internal tables
    ap_uint<36> fseStateBitsTableLL[512];
    uint16_t fseNextStateTableLL[512];
    ap_uint<36> fseStateBitsTableML[512];
    uint16_t fseNextStateTableML[512];
    ap_uint<36> fseStateBitsTableOF[256];
    uint16_t fseNextStateTableOF[256];
    // out word having D-WIDTH = (9 (max tableLog) * 3) + 1(end bit)
    IntVectorStream_dt<28, 1> fseOutWord;
fse_encode_seq_main:
    while (true) {
        uint8_t tableLogLL, tableLogML, tableLogOF;
        uint16_t maxSymbolLL, maxSymbolML, maxSymbolOF;
        uint16_t maxFreqLL, maxFreqML, maxFreqOF;
        // read initial value to check OES
        // read FSE encoding tables for litlen, matlen, offset
        bool noData = readFseTable(fseSeqTableStream, fseStateBitsTableLL, fseNextStateTableLL, tableLogLL, maxFreqLL);
        if (noData) break;
        readFseTable(fseSeqTableStream, fseStateBitsTableOF, fseNextStateTableOF, tableLogOF, maxFreqOF);
        readFseTable(fseSeqTableStream, fseStateBitsTableML, fseNextStateTableML, tableLogML, maxFreqML);
        // Check for no sequence condition
        auto noSeq = noSeqFlagStream.read();
        if (noSeq) continue;
        // read and fse encode sequence codes
        uint16_t llPrevStateVal, mlPrevStateVal, ofPrevStateVal;
        ap_uint<9> outWordLL, outWordML, outWordOF;
        ap_uint<5> bitsLL, bitsML, bitsOF;
        // Initialise fse states for first sequence set
        auto seqCode = seqCodeStream.read();
        uint8_t llCode = (uint8_t)seqCode.data[0].litlen;
        uint8_t ofCode = (uint8_t)seqCode.data[0].offset;
        uint8_t mlCode = (uint8_t)seqCode.data[0].matlen;
        // Initialization does not write any output
        fseEncodeSymbol<9, 1>(ofCode, fseStateBitsTableOF, fseNextStateTableOF, ofPrevStateVal, outWordOF, bitsOF);
        fseEncodeSymbol<9, 1>(mlCode, fseStateBitsTableML, fseNextStateTableML, mlPrevStateVal, outWordML, bitsML);
        fseEncodeSymbol<9, 1>(llCode, fseStateBitsTableLL, fseNextStateTableLL, llPrevStateVal, outWordLL, bitsLL);
        uint8_t tableLogSum = tableLogOF + tableLogML + tableLogLL;
        ap_uint<28> endMark = (1 << tableLogSum);
        ap_uint<5> fseBitCnt = 0;
        fseOutWord.strobe = 1;
    fse_encode_seq_codes:
        for (seqCode = seqCodeStream.read(); seqCode.strobe > 0; seqCode = seqCodeStream.read()) {
#pragma HLS PIPELINE II = 1
            ofCode = (uint8_t)seqCode.data[0].offset;
            mlCode = (uint8_t)seqCode.data[0].matlen;
            llCode = (uint8_t)seqCode.data[0].litlen;
            fseEncodeSymbol<9, 0>(ofCode, fseStateBitsTableOF, fseNextStateTableOF, ofPrevStateVal, outWordOF, bitsOF);
            fseEncodeSymbol<9, 0>(mlCode, fseStateBitsTableML, fseNextStateTableML, mlPrevStateVal, outWordML, bitsML);
            fseEncodeSymbol<9, 0>(llCode, fseStateBitsTableLL, fseNextStateTableLL, llPrevStateVal, outWordLL, bitsLL);
            // Prepare output
            fseOutWord.data[0] =
                ((ap_uint<28>)outWordLL << (bitsOF + bitsML)) + ((ap_uint<28>)outWordML << bitsOF) + outWordOF;
            fseBitCnt = bitsOF + bitsML + bitsLL;
            // Write output to stream
            seqFseWordStream << fseOutWord;
            wordBitlenStream << fseBitCnt;
        }
        // encode last sequence states
        outWordML = mlPrevStateVal & c_bitMask[tableLogML];
        outWordOF = ofPrevStateVal & c_bitMask[tableLogOF];
        outWordLL = llPrevStateVal & c_bitMask[tableLogLL];
        // prepare last output
        fseOutWord.data[0] = ((ap_uint<28>)outWordLL << (tableLogOF + tableLogML)) +
                             ((ap_uint<18>)outWordOF << tableLogML) + outWordML + endMark;
        fseBitCnt = 1 + tableLogSum;
        // write last valid output for this block
        seqFseWordStream << fseOutWord;
        wordBitlenStream << fseBitCnt;
        // End of block
        fseOutWord.strobe = 0;
        seqFseWordStream << fseOutWord;
    }
    // End of all data
    fseOutWord.strobe = 0;
    seqFseWordStream << fseOutWord;
}

// Content of called function
void fseGetSeqCodes(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                    hls::stream<DSVectorStream_dt<Sequence_dt<6, 6, 6>, 1> >& seqCodeStream,
                    hls::stream<bool>& noSeqFlagStream,
                    hls::stream<ap_uint<33> >& extCodewordStream,
                    hls::stream<ap_uint<8> >& extBitlenStream) {
    // get sequence, code and code bit-lengths
    DSVectorStream_dt<Sequence_dt<6, 6, 6>, 1> seqCode;
    ap_uint<33> extCodeword;
    ap_uint<8> extBitlen;
fse_get_seq_codes_main:
    while (true) {
        auto nextSeq = inSeqStream.read();
        if (nextSeq.strobe == 0) break;
        seqCode.strobe = 1;
        // check for noSeq condition
        if (nextSeq.data[0].litlen == 0 && nextSeq.data[0].matlen == 0 && nextSeq.data[0].offset == 0) {
            // read strobe zero value, since no sequence is present
            nextSeq = inSeqStream.read();
            noSeqFlagStream << 1;
        } else {
            noSeqFlagStream << 0;
        // Send sequence codes and extra bit-lengths with extra codewords
        fetch_sequence_codes:
            while (nextSeq.strobe > 0) {
#pragma HLS PIPELINE II = 1
                auto inSeq = nextSeq;
                // Read next sequence
                nextSeq = inSeqStream.read();
                auto litLen = inSeq.data[0].litlen;
                auto matLen = inSeq.data[0].matlen;
                auto offset = inSeq.data[0].offset;
                // process current sequence
                seqCode.data[0].litlen = getLLCode(litLen);
                seqCode.data[0].matlen = getMLCode(matLen);
                seqCode.data[0].offset = bitsUsed31(offset);
                // get bits for adding to bitstream
                uint8_t llBits = c_extraBitsLL[seqCode.data[0].litlen];
                uint8_t mlBits = c_extraBitsML[seqCode.data[0].matlen];
                uint8_t ofBits = seqCode.data[0].offset;
                // get masked extra bit values
                ap_uint<33> excLL = litLen & c_bitMask[llBits];
                ap_uint<33> excML = matLen & c_bitMask[mlBits];
                ap_uint<33> excOF = offset & c_bitMask[ofBits];
                // get combined extra codeword
                extCodeword = (excOF << (mlBits + llBits)) + (excML << llBits) + excLL;
                extBitlen = ofBits + mlBits + llBits;
                // write information to next units
                seqCodeStream << seqCode;
                extCodewordStream << extCodeword;
                extBitlenStream << extBitlen;
            }
            // End of block in case of valid sequence block
            seqCode.strobe = 0;
            seqCodeStream << seqCode;
        }
    }
}

// Content of called function
void seqFseBitPacker(hls::stream<IntVectorStream_dt<28, 1> >& seqFseWordStream,
                     hls::stream<ap_uint<5> >& fseWordBitlenStream,
                     hls::stream<ap_uint<33> >& extCodewordStream,
                     hls::stream<ap_uint<8> >& extBitlenStream,
                     hls::stream<IntVectorStream_dt<8, 8> >& encodedOutStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& seqEncSizeStream) {
    // generate fse bitstream for sequences
    constexpr uint8_t c_outBytes = 8;
    constexpr uint8_t c_outBits = 64;
    IntVectorStream_dt<8, 8> outVal;

seq_fse_bitPack_outer:
    while (true) {
        auto seqFseWord = seqFseWordStream.read();
        if (seqFseWord.strobe == 0) break;
        // local buffer
        ap_uint<128> bitstream = 0;
        int8_t bitCount = 0;
        ap_uint<MAX_FREQ_DWIDTH> outCnt = 0;
        // 8 bytes in an output word
        outVal.strobe = c_outBytes;
    // pack fse bitstream
    seq_fse_bit_packing:
        for (; seqFseWord.strobe > 0; seqFseWord = seqFseWordStream.read()) {
#pragma HLS PIPELINE II = 1
            // add extra bit word and then fse word
            // Read input
            auto extWord = (ap_uint<128>)extCodewordStream.read();
            auto extBlen = extBitlenStream.read();
            auto fseWord = (ap_uint<128>)seqFseWord.data[0];
            auto fseBlen = fseWordBitlenStream.read();

            bitstream += (fseWord << (extBlen + bitCount)) + (extWord << bitCount);
            bitCount += (extBlen + fseBlen);
            // push bitstream
            if (bitCount > 63) {
                // write to output stream
                for (uint8_t k = 0; k < c_outBytes; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = bitstream.range(7 + (k * 8), k * 8);
                }
                encodedOutStream << outVal;
                bitstream >>= c_outBits;
                bitCount -= c_outBits;
                outCnt += c_outBytes;
            }
        }
        // write remaining bitstream
        if (bitCount) {
            // write to output stream
            for (uint8_t k = 0; k < c_outBytes; ++k) {
#pragma HLS UNROLL
                outVal.data[k] = bitstream.range(7 + (k * 8), k * 8);
            }
            outVal.strobe = (uint8_t)((bitCount + 7) / 8);
            ;
            encodedOutStream << outVal;
            outCnt += outVal.strobe;
        }
        // printf("seqbitpacker written bytes: %u\n", (uint16_t)outCnt);
        // end of block
        outVal.strobe = 0;
        encodedOutStream << outVal;
        // send size of encoded sequence bitstream
        seqEncSizeStream << outCnt;
    }
    // end of all data
    outVal.strobe = 0;
    encodedOutStream << outVal;
}