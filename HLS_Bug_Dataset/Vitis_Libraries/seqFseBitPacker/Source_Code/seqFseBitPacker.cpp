void seqFseBitPacker(hls::stream<IntVectorStream_dt<28, 1> >& seqFseWordStream,
                     hls::stream<ap_uint<5> >& fseWordBitlenStream,
                     hls::stream<ap_uint<33> >& extCodewordStream,
                     hls::stream<ap_uint<8> >& extBitlenStream,
                     hls::stream<ap_uint<68> >& encodedOutStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& seqEncSizeStream) {
    // generate fse bitstream for sequences
    constexpr uint8_t c_outBytes = 8;
    constexpr uint8_t c_outBits = 64;
    ap_uint<68> outVal;

seq_fse_bitPack_outer:
    while (true) {
        auto seqFseWord = seqFseWordStream.read();
        if (seqFseWord.strobe == 0) break;
        // local buffer
        ap_uint<128> bitstream = 0;
        int8_t bitCount = 0;
        ap_uint<MAX_FREQ_DWIDTH> outCnt = 0;
        // 8 bytes in an output word
        outVal.range(3, 0) = c_outBytes;
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
                outVal.range(c_outBits + 3, 4) = bitstream.range(c_outBits - 1, 0);
                encodedOutStream << outVal;
                bitstream >>= c_outBits;
                bitCount -= c_outBits;
                outCnt += c_outBytes;
            }
        }
        // write remaining bitstream
        if (bitCount) {
            // write to output stream
            uint8_t outStrobe = (uint8_t)((bitCount + 7) / 8);
            outVal.range(c_outBits + 3, 4) = bitstream.range(c_outBits - 1, 0);
            outVal.range(3, 0) = outStrobe;
            encodedOutStream << outVal;
            outCnt += outStrobe;
        }
        // printf("seqbitpacker written bytes: %u\n", (uint16_t)outCnt);
        // send size of encoded sequence bitstream
        seqEncSizeStream << outCnt;
        // end of block
        outVal = 0;
        encodedOutStream << outVal;
    }
    // end of all data
    outVal = 0;
    encodedOutStream << outVal;
}