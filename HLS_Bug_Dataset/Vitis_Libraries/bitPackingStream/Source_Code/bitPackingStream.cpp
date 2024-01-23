void bitPackingStream(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 2> >& outStream) {
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> inValue;
    IntVectorStream_dt<8, 2> outValue;
    int blk_n = 0;
    while (true) { // iterate over blocks
        ap_uint<32> localBits = 0;
        uint8_t localBits_idx = 0;
        bool last_block = true;

    bitpack:
        for (inValue = inStream.read(); inValue.strobe != 0; inValue = inStream.read()) {
#pragma HLS PIPELINE II = 1
            uint16_t val = inValue.data[0].code;
            uint8_t size = inValue.data[0].bitlen;
            localBits.range(size + localBits_idx - 1, localBits_idx) = val;
            localBits_idx += size;

            if (localBits_idx >= 16) {
                uint16_t pack_byte = localBits;
                localBits >>= 16;
                localBits_idx -= 16;

                outValue.strobe = 2;
                outValue.data[0] = (uint8_t)pack_byte;
                outValue.data[1] = (uint8_t)(pack_byte >> 8);
                outStream << outValue;
            }
            last_block = false;
        }

        // end of file
        if (last_block) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }

        int leftBits = localBits_idx % 8;
        int val = 0;
        ap_uint<64> packedBits = 0;

        if (leftBits) {
            // 3 bit header
            localBits_idx += 3;

            leftBits = localBits_idx % 8;

            if (leftBits != 0) {
                val = 8 - leftBits;
                localBits_idx += val;
            }

            // Zero bytes and complement as per type0 z_sync_flush
            uint64_t val = 0xffff0000;
            packedBits = localBits | (val << localBits_idx);
            localBits_idx += 32;

        } else {
            // Zero bytes and complement as per type0 z_sync_flush
            uint64_t val = 0xffff000000;
            packedBits = localBits | (val << localBits_idx);
            localBits_idx += 40;
        }
        for (uint32_t i = 0; i < localBits_idx; i += 16) {
#pragma HLS PIPELINE II = 1
            uint16_t pack_byte = packedBits;
            packedBits >>= 16;

            outValue.data[0] = pack_byte;
            if (localBits_idx - i < 9) {
                outValue.strobe = 1;
            } else {
                outValue.strobe = 2;
                outValue.data[1] = (uint8_t)(pack_byte >> 8);
            }
            outStream << outValue;
        }
        // end of block
        outValue.strobe = 0;
        outValue.data[0] = 0;
        outStream << outValue;
    }
}