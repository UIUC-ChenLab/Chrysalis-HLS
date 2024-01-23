void zstdHuffmanEncoder(hls::stream<IntVectorStream_dt<8, 1> >& inValStream,
                        hls::stream<bool>& rleFlagStream,
                        hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& hfCodeStream,
                        hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& hfEncodedStream,
                        hls::stream<ap_uint<16> >& hfLitMetaStream) {
    // read huffman table
    HuffmanCode_dt<MAX_BITS> hfcTable[256]; // use LUTs for implementation as registers
#pragma HLS aggregate variable = hfcTable
#pragma HLS BIND_STORAGE variable = hfcTable type = ram_1p impl = lutram
    //#pragma HLS ARRAY_PARTITION variable = hfcTable complete
    DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> outVal;
    bool done = false;

    while (true) {
        ap_uint<16> streamSizes[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSizes complete
        // pre-read value to check strobe
        auto inVal = inValStream.read();
        if (inVal.strobe == 0) break;
        // Check if this literal block is RLE type
        bool isRle = rleFlagStream.read();
        uint8_t streamCnt = inVal.data[0];
        hfLitMetaStream << ((uint16_t)streamCnt & 0x000F);
    // read the stream sizes
    get_lit_streams_size:
        for (uint8_t si = 0; si < streamCnt; ++si) {
#pragma HLS PIPELINE II = 2
            // read present stream size
            inVal = inValStream.read();
            streamSizes[si].range(7, 0) = inVal.data[0];
            inVal = inValStream.read();
            streamSizes[si].range(15, 8) = inVal.data[0];
        }
        // read this table only once
        uint16_t hIdx = 0;
    read_hfc_table:
        for (auto hfCode = hfCodeStream.read(); hfCode.strobe > 0; hfCode = hfCodeStream.read()) {
#pragma HLS PIPELINE II = 1
            hfcTable[hIdx] = hfCode.data[0];
            ++hIdx;
        }
    // Parallel read 8 * INSTANCES bits of input
    // Parallel encode to INSTANCES output streams
    encode_lit_streams:
        for (uint8_t si = 0; si < streamCnt; ++si) {
            auto streamSize = streamSizes[si];
            uint16_t streamCmpSize = 0;
            uint8_t cmpBits = 0;
            // Since for RLE type literals, only one stream is present
            // in that stream, last literal must not be encoded, comes first in reversed stream
            if (isRle) {
                --streamSize;
                inValStream.read();
            }
            outVal.strobe = 1;
        huff_encode:
            for (uint16_t i = 0; i < (uint16_t)streamSize; ++i) {
#pragma HLS PIPELINE II = 1
                inVal = inValStream.read();
                outVal.data[0] = hfcTable[inVal.data[0]];
                hfEncodedStream << outVal;
                cmpBits += outVal.data[0].bitlen;
                if (cmpBits > 7) {
                    streamCmpSize += cmpBits >> 3;
                    cmpBits &= 7;
                }
            }
            // cmpBits cannot be greater than 7, 1 extra bit indicating end of stream
            if (cmpBits + 1) {
                ++streamCmpSize;
            }
            hfLitMetaStream << streamCmpSize;
            // end of sub-stream
            outVal.strobe = 0;
            hfEncodedStream << outVal;
        }
    }
    // end of file
    hfCodeStream.read();
    outVal.strobe = 0;
    hfEncodedStream << outVal;
}