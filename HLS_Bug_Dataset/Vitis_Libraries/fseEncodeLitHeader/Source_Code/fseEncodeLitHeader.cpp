void fseEncodeLitHeader(hls::stream<IntVectorStream_dt<4, 1> >& hufWeightStream,
                        hls::stream<IntVectorStream_dt<36, 1> >& fseLitTableStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& encodedOutStream) {
    // fse encoding of huffman header for encoded literals
    IntVectorStream_dt<8, 2> outVal;
    ap_uint<36> fseStateBitsTable[256];
    uint16_t fseNextStateTable[256];
    ap_uint<4> hufWeights[256];
#pragma HLS BIND_STORAGE variable = hufWeights type = ram_1p impl = lutram

fse_lit_encode_outer:
    while (true) {
        uint8_t tableLog;
        uint16_t maxSymbol;
        uint16_t maxFreq;
        uint16_t fIdx = 0;
        // read details::c_maxLitV + 1 data from huffman weight stream
        auto inWeight = hufWeightStream.read();
        if (inWeight.strobe == 0) break;
    read_hf_weights:
        for (; inWeight.strobe > 0; inWeight = hufWeightStream.read()) {
#pragma HLS PIPELINE II = 1
            hufWeights[fIdx] = inWeight.data[0];
            if (inWeight.data[0] > 0) maxSymbol = fIdx;
            ++fIdx;
        }
        // read FSE encoding tables
        readFseTable(fseLitTableStream, fseStateBitsTable, fseNextStateTable, tableLog, maxFreq);
        uint16_t preStateVal[2];
        bool isInit[2] = {true, true};
        bool stateIdx = 0; // 0 for even, 1 for odd
        uint8_t bitCount = 0;
        ap_uint<32> bitstream = 0;
        outVal.strobe = 2;
        int outCnt = 0;
        // encode weights stored in reverse order
        stateIdx = maxSymbol & 1;
    fse_lit_encode:
        for (int16_t w = maxSymbol - 1; w > -1; --w) { // TODO: Fast forward to 2 symbols per clock cycle
#pragma HLS PIPELINE II = 1
            uint8_t symbol = hufWeights[w];
            fseEncodeSymbol<32>(symbol, fseStateBitsTable, fseNextStateTable, preStateVal[stateIdx], bitstream,
                                bitCount, isInit[stateIdx]);
            isInit[stateIdx] = false;
            // write bitstream to output
            if (bitCount > 15) {
                // write to output stream
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                encodedOutStream << outVal;
                bitstream >>= 16;
                bitCount -= 16;
                outCnt += 2;
            }
            // switch state flow
            stateIdx = (stateIdx + 1) & 1; // 0 if 1, 1 if 0
        }
        // encode last two
        bitstream |= ((ap_uint<32>)(preStateVal[0] & c_bitMask[tableLog]) << bitCount);
        bitCount += tableLog;
        bitstream |= ((ap_uint<32>)(preStateVal[1] & c_bitMask[tableLog]) << bitCount);
        bitCount += tableLog;
        // mark end by adding 1-bit "1"
        bitstream |= (ap_uint<32>)1 << bitCount;
        ++bitCount;
        // max remaining biCount can be 15 + (2 * 6) + 1= 28 bits => 4 bytes
        int8_t remBytes = (int8_t)((bitCount + 7) / 8);
    // write bitstream to output
    write_rem_bytes:
        while (remBytes > 0) {
#pragma HLS PIPELINE II = 1
            // write to output stream
            outVal.data[0] = bitstream.range(7, 0);
            outVal.data[1] = bitstream.range(15, 8);
            outVal.strobe = ((remBytes > 1) ? 2 : 1);
            encodedOutStream << outVal;
            bitstream >>= 16;
            remBytes -= 2;
            outCnt += outVal.strobe;
        }
        outVal.strobe = 0;
        encodedOutStream << outVal;
    }
    // dump strobe 0 data
    fseLitTableStream.read();
    // write end of block
    outVal.strobe = 0;
    encodedOutStream << outVal;
}