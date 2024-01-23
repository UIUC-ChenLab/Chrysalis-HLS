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