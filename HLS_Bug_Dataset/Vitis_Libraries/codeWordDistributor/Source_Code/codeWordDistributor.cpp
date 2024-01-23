void codeWordDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& inStreamCode,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode1,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outStreamCode2,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& inStreamMaxCode,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode1,
                         hls::stream<ap_uint<c_tgnSymbolBits> >& outStreamMaxCode2,
                         hls::stream<bool>& isEOBlocks) {
    const int hctMeta[2] = {c_litCodeCount, c_dstCodeCount};
    while (isEOBlocks.read() == false) {
    distribute_litdist_codes:
        for (uint8_t i = 0; i < 2; i++) {
            auto maxCode = inStreamMaxCode.read();
            outStreamMaxCode1 << maxCode;
            outStreamMaxCode2 << maxCode;
        distribute_hufcodes_main:
            for (uint16_t j = 0; j < hctMeta[i]; j++) {
#pragma HLS PIPELINE II = 1
                auto inVal = inStreamCode.read();
                outStreamCode1 << inVal;
                outStreamCode2 << inVal;
            }
        }
    }
}