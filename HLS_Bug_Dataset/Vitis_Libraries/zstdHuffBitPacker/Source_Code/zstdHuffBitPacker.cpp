void zstdHuffBitPacker(hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& hfEncodedStream,
                       hls::stream<IntVectorStream_dt<8, 2> >& hfBitStream) {
    // pack huffman codes from multiple input code streams
    bool done = false;
    IntVectorStream_dt<8, 2> outVal;
    ap_uint<32> outReg = 0;
    while (!done) {
        done = true;
        outVal.strobe = 2;
        uint8_t validBits = 0;
        uint16_t outCnt = 0;
    hf_bitPacking:
        for (auto inVal = hfEncodedStream.read(); inVal.strobe > 0; inVal = hfEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            done = false;
            outReg.range((uint8_t)(inVal.data[0].bitlen) + validBits - 1, validBits) = inVal.data[0].code;
            validBits += (uint8_t)(inVal.data[0].bitlen);
            if (validBits > 15) {
                validBits -= 16;
                outVal.data[0] = outReg.range(7, 0);
                outVal.data[1] = outReg.range(15, 8);
                // outVal.data[2] = outReg.range(23, 16);
                // outVal.data[3] = outReg.range(31, 24);
                outReg >>= 16;
                hfBitStream << outVal;
                outCnt += 2;
            }
        }
        if (outCnt) {
            // mark end by adding 1-bit "1"
            outReg.range(validBits, validBits) = 1;
            ++validBits;
        }
        // write remaining bits
        if (validBits) {
            outVal.strobe = (validBits + 7) >> 3;
            outVal.data[0] = outReg.range(7, 0);
            outVal.data[1] = outReg.range(15, 8);
            // outVal.data[2] = outReg.range(23, 16);
            // outVal.data[3] = outReg.range(31, 24);
            hfBitStream << outVal;
            outCnt += outVal.strobe;
        }
        // printf("bitpacker written bytes: %u\n", outCnt);
        outVal.strobe = 0;
        hfBitStream << outVal;
    }
}