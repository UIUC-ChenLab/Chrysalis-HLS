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