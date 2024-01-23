void hfdDataFeader(hls::stream<ap_uint<IN_BYTES * 8> >& inStream,
                   uint8_t streamCnt,
                   uint16_t* cmpSize,
                   ap_uint<IN_BYTES * 8 * 2> accHuff,
                   uint8_t bytesInAcc,
                   hls::stream<IntVectorStream_dt<BS_WIDTH, 1> >& huffBitStream,
                   hls::stream<ap_uint<8> >& validBitCntStream) {
    const uint8_t c_inputByte = IN_BYTES;
    const uint16_t c_streamWidth = 8 * c_inputByte;
    const uint16_t c_BSWidth = BS_WIDTH;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const int c_bsPB = c_BSWidth / 8;
    const int c_bsUpperLim = (((32 / c_bsPB) / 2) * 1024);

    ap_uint<c_BSWidth> bitStream[c_bsUpperLim];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_t2p impl = bram

    // internal registers
    ap_uint<c_accRegWidth> bsbuff = accHuff; // must not contain more than 3 bytes
    uint8_t bitsInAcc = bytesInAcc * 8;

    int wIdx = 0;
    int rIdx = 0;
    int fmInc = 1; // can be +1 or -1
    int smInc = 1;
    uint8_t bitsWritten = c_BSWidth;
    int streamRBgn[4];     // starting index for BRAM read
    int streamRLim[4 + 1]; // point/index till which the BRAM can be read, 1 extra buffer entry
#pragma HLS ARRAY_PARTITION variable = streamRBgn complete
#pragma HLS ARRAY_PARTITION variable = streamRLim complete
    uint8_t wsi = 0, rsi = 0;
    int inIdx = 0;
    // modes
    bool fetchMode = 1;
    bool streamMode = 0;
    bool done = 0;
    // initialize
    streamRLim[wsi] = 0; // initial stream will stream from higher to lower address
hfdl_dataStreamer:
    while (!done) {
#pragma HLS PIPELINE II = 1
        // stream data, bitStream buffer width is equal to inStream width for simplicity
        if (fetchMode) {
            // fill bitstream in direction specified by increment variable
            if (inIdx + bytesInAcc < cmpSize[wsi] && bytesInAcc < c_bsPB) {
                bsbuff.range(bitsInAcc + c_streamWidth - 1, bitsInAcc) = inStream.read();
                bitsInAcc += c_streamWidth;
            }
            bitStream[wIdx] = bsbuff.range(c_BSWidth - 1, 0);
            if (inIdx + c_bsPB >= cmpSize[wsi]) {
                auto bw = 8 * (cmpSize[wsi] - inIdx);
                bitsWritten = (bw == 0) ? bitsWritten : bw;
                validBitCntStream << bitsWritten;
                bsbuff >>= bitsWritten;
                bitsInAcc -= bitsWritten;
                bytesInAcc = bitsInAcc >> 3;

                // update fetch mode state
                if (streamMode == 0) {
                    streamMode = 1;
                    rIdx = wIdx;
                }
                inIdx = 0;            // just an index, not directional
                fmInc = (~fmInc) + 1; // flip 1 and -1
                streamRBgn[wsi] = wIdx;
                ++wsi;
                if (wsi & 1) {
                    streamRLim[wsi] = c_bsUpperLim - 1;
                    wIdx = c_bsUpperLim - 1;
                } else {
                    streamRLim[wsi] = 0;
                    wIdx = 0;
                }
                // post increment checks
                if ((wsi == streamCnt) || (wsi - rsi > 1)) fetchMode = 0;
                // reset default value
                bitsWritten = c_BSWidth;
                continue;
            }
            bsbuff >>= bitsWritten;
            bitsInAcc -= bitsWritten;
            bytesInAcc = bitsInAcc >> 3;

            inIdx += c_bsPB;
            wIdx += fmInc;
        }
        if (streamMode) {
            // write data to output stream
            uint32_t tmp = bitStream[rIdx];
            // output register
            IntVectorStream_dt<BS_WIDTH, 1> outValue;
            outValue.data[0] = tmp;
            // update stream mode state
            if (rIdx == streamRLim[rsi]) {
                outValue.strobe = 0;
                ++rsi;
                rIdx = streamRBgn[rsi];
                smInc = (~smInc) + 1; // flip 1 and -1
                // no need to check if fetchMode == 0
                if (wsi < streamCnt) fetchMode = 1;
                // either previous streamMode ended quicker than next fetchMode or streamCnt reached
                if (wsi == rsi) streamMode = 0;
            } else {
                outValue.strobe = 1;
                rIdx -= smInc;
            }
            huffBitStream << outValue;
        }
        // end condition
        if (!(fetchMode | streamMode)) done = 1;
    }
}