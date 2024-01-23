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