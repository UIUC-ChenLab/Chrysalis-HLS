void readfseS2Bram(uint8_t* fseHdrBuf,
                   uint8_t& fseHIdx,
                   uint16_t& hdrBsLen,
                   IntVectorStream_dt<8, 2>& fhVal,
                   hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream) {
#pragma HLS inline off
buffer_llofml_fsebs: // taking 1.3K LUTs
    for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
        fseHdrBuf[fseHIdx] = fhVal.data[0];
        fseHdrBuf[fseHIdx + 1] = fhVal.data[1];
        fseHIdx += fhVal.strobe;
        hdrBsLen += fhVal.strobe;
    }
}