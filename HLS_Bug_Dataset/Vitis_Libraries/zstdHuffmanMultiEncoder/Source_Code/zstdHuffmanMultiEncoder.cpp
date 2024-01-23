void zstdHuffmanMultiEncoder(
    hls::stream<ap_uint<68> >& inValStream,
    hls::stream<bool>& rleFlagStream,
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& hfCodeStream,
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, PARALLEL_HUFFMAN> >& hfEncodedStream,
    hls::stream<ap_uint<16> >& hfLitMetaStream) {
    // read huffman table
    constexpr uint8_t c_hftCount = PARALLEL_HUFFMAN / 2;
    constexpr uint8_t c_encPerRead = 8 / PARALLEL_HUFFMAN;
    HuffmanCode_dt<MAX_BITS> hfcTable[c_hftCount][256];
#pragma HLS aggregate variable = hfcTable
#pragma HLS BIND_STORAGE variable = hfcTable type = ram_2p impl = lutram
    DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, PARALLEL_HUFFMAN> outVal;
    bool done = false;
    HuffmanCode_dt<MAX_BITS> zeroHf;
    zeroHf.code = 0;
    zeroHf.bitlen = 0;

    while (true) {
        uint16_t streamSizes[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSizes complete
        // pre-read value to check strobe
        auto inVal = inValStream.read();
        if (inVal.range(3, 0) == 0) break;
        // Check if this literal block is RLE type
        bool isRle = rleFlagStream.read();
        uint8_t streamCnt = inVal.range(11, 4);
        hfLitMetaStream << ((uint16_t)streamCnt & 0x000F);
        // read the stream sizes
        inVal = inValStream.read();
    get_lit_streams_size:
        for (uint8_t si = 0; si < streamCnt; ++si) {
#pragma HLS UNROLL
            // read present stream size
            streamSizes[si] = inVal.range(15 + (si * 16) + 4, (si * 16) + 4);
        }
        // read this table only once
        uint16_t hIdx = 0;
    read_hfc_table:
        for (auto hfCode = hfCodeStream.read(); hfCode.strobe > 0; hfCode = hfCodeStream.read()) {
#pragma HLS PIPELINE II = 1
            for (uint8_t k = 0; k < c_hftCount; ++k) {
#pragma HLS UNROLL
                hfcTable[k][hIdx] = hfCode.data[0];
            }
            ++hIdx;
        }
    // Read 64-bits of input
    // Parallel encode to PARALLEL_HUFFMAN output streams
    encode_lit_streams:
        for (uint8_t si = 0; si < streamCnt; ++si) {
            auto streamSize = streamSizes[si];
            uint16_t streamCmpSize = 0;
            uint8_t cmpBits = 0;
            // bool readFlag = false;
            inVal = inValStream.read();
            ap_uint<64> inLit = inVal.range(67, 4);
            // check if strobe is less, since stream is in reverse, will have to dump data from LSB
            uint8_t readInc = 0;
            uint8_t pInc = PARALLEL_HUFFMAN;
            uint8_t strb = inVal.range(3, 0);
            if (strb < 8) {
                inLit >>= (8 * (8 - strb));
            }
            // Since for RLE type literals, only one stream is present
            // in that stream, last literal must not be encoded, comes first in reversed stream
            if (isRle) {
                --streamSize;
                inLit >>= 8;
            }
        huff_encode:
            for (int i = 0; i < (int)streamSize; i += pInc) {
#pragma HLS PIPELINE II = 1
                if ((i > 0) && (readInc == c_encPerRead)) {
                    inVal = inValStream.read();
                    inLit = inVal.range(67, 4);
                    strb = inVal.range(3, 0);
                    readInc = 0;
                }
                for (uint8_t k = 0; k < PARALLEL_HUFFMAN; ++k) {
#pragma HLS UNROLL
                    const uint8_t tblIdx = k >> 1;
                    if (i + k < streamSize && k < strb) { // can manage incorrect strobe at the end of stream
                        outVal.data[k] = hfcTable[tblIdx][inLit.range(7 + (k * 8), k * 8)];
                        cmpBits += outVal.data[k].bitlen;
                    } else {
                        outVal.data[k] = zeroHf; // assign zero code & bitlen, helps in bitpacking
                    }
                }
                outVal.strobe = ((i < (int)streamSize - PARALLEL_HUFFMAN) ? (unsigned)PARALLEL_HUFFMAN
                                                                          : (unsigned)(streamSize - i));
                hfEncodedStream << outVal;
                inLit >>= (8 * PARALLEL_HUFFMAN);
                pInc = ((strb > PARALLEL_HUFFMAN) ? PARALLEL_HUFFMAN : strb);
                strb -= pInc;
                ++readInc;
                if (cmpBits > 7) {
                    streamCmpSize += cmpBits >> 3;
                    cmpBits &= 7;
                }
            }
            // printf("Read %d bytes, Written %d bytes\n", rb, wb);
            // cmpBits cannot be greater than 7, 1 extra bit indicating end of stream
            ++streamCmpSize;
            hfLitMetaStream << streamCmpSize;
            // end of block
            outVal.strobe = 0;
            hfEncodedStream << outVal;
        }
    }
    // end of file
    hfCodeStream.read();
    outVal.strobe = 0;
    hfEncodedStream << outVal;
}