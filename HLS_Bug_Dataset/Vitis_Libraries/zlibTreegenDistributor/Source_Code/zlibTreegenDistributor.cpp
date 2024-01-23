void zlibTreegenDistributor(
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[NUM_BLOCK],
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufSerialCodeStream[NUM_TREEGEN],
    hls::stream<uint8_t>& inIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> hufCodeOut;
    static uint8_t treeIdx = 0;

tgndst_units_main:
    for (uint8_t i = inIdxNum.read(); i != 0xFF; i = inIdxNum.read()) {
    tgndst_litDist:
        for (uint16_t j = 0; j < c_litDistCodeCnt; j++) {
#pragma HLS PIPELINE II = 1
            hufCodeOut = hufSerialCodeStream[treeIdx].read();
            hufCodeStream[i] << hufCodeOut;
        }
    tgndst_bitlen:
        while (1) {
#pragma HLS PIPELINE II = 1
            hufCodeOut = hufSerialCodeStream[treeIdx].read();
            hufCodeStream[i] << hufCodeOut;
            if (hufCodeOut.data[0].bitlen == 0) break;
        }
        treeIdx++;
        if (treeIdx == NUM_TREEGEN) treeIdx = 0;
    }
    // send eos to all unit
    hufCodeOut.strobe = 0;
tgndst_send_eos:
    for (uint8_t i = 0; i < NUM_BLOCK; ++i) {
        hufCodeStream[i] << hufCodeOut;
    }
}