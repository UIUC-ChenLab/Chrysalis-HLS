void fseDecodeStatesLL(hls::stream<uint8_t>& litsymbolStream,
                       hls::stream<uint8_t>& mlsymbolStream,
                       hls::stream<uint8_t>& ofsymbolStream,
                       hls::stream<ap_uint<32> >& litbsWordStream,
                       hls::stream<ap_uint<32> >& mlbsWordStream,
                       hls::stream<ap_uint<32> >& ofbsWordStream,
                       hls::stream<ap_uint<5> >& litextraBitStream,
                       hls::stream<ap_uint<5> >& mlextraBitStream,
                       uint32_t seqCnt,
                       uint32_t litCnt,
                       ap_uint<LMO_WIDTH>* prevOffsets,
                       hls::stream<ap_uint<LMO_WIDTH> >& litLenStream,
                       hls::stream<ap_uint<LMO_WIDTH> >& offsetStream,
                       hls::stream<ap_uint<LMO_WIDTH> >& matLenStream,
                       hls::stream<bool>& litlenValidStream) {
    // calculate literal length, match length and offset values, stream them for sequence execution
    ap_uint<LMO_WIDTH> offsetVal;
    ap_uint<LMO_WIDTH> litLenCode;
    uint8_t ofi;
    bool checkLL = false;
    litbsWordStream.read(); // dump first word
decode_sequence_codes:
    while (seqCnt) {
#pragma HLS PIPELINE II = 1
        // calculate offset and set prev offsets
        auto symbol = ofsymbolStream.read();
        auto bsWord = ofbsWordStream.read();
        auto extBit = symbol;
        uint32_t extVal = bsWord & (((uint64_t)1 << extBit) - 1);
        offsetVal = (1 << symbol) + extVal;

        ofi = 3;
        if (offsetVal > 3) {
            offsetVal -= 3;
            checkLL = false;
        } else {
            checkLL = true;
        }
        // calculate match length
        symbol = mlsymbolStream.read();
        bsWord = mlbsWordStream.read();
        extBit = mlextraBitStream.read();
        uint16_t extVal1 = (bsWord & (((uint64_t)1 << extBit) - 1));
        ap_uint<LMO_WIDTH> matchLenCode = c_baseML[symbol] + extVal1;
        matLenStream << matchLenCode;

        // calculate literal length
        symbol = litsymbolStream.read();
        bsWord = litbsWordStream.read();
        extBit = litextraBitStream.read();
        extVal1 = (bsWord & (((uint64_t)1 << extBit) - 1));
        litLenCode = c_baseLL[symbol] + extVal1;
        litlenValidStream << 1;
        litLenStream << litLenCode;
        litCnt -= litLenCode;

        // update offset as per literal length
        if (checkLL) {
            // repeat offsets 1 - 3
            if (litLenCode == 0) {
                if (offsetVal == 3) {
                    offsetVal = prevOffsets[0] - 1;
                    ofi = 2;
                } else {
                    ofi = offsetVal;
                    offsetVal = prevOffsets[offsetVal];
                }
            } else {
                ofi = offsetVal - 1;
                offsetVal = prevOffsets[offsetVal - 1];
            }
        }
        checkLL = false;
        // OFFSET_WNU: write offset and update previous offsets
        offsetStream << offsetVal;
        // shift previous offsets
        auto prev1 = prevOffsets[1];
        if (ofi > 1) {
            prevOffsets[2] = prev1;
        }
        if (ofi > 0) {
            prevOffsets[1] = prevOffsets[0];
            prevOffsets[0] = offsetVal;
        }
        --seqCnt;
    }
    if (litCnt > 0) {
        litlenValidStream << 1;
        litLenStream << litCnt;
        matLenStream << 0;
        offsetStream << 0;
    }
}