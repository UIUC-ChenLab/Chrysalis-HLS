void s2mmStreamSimple(ap_uint<DATAWIDTH>* out,
                      hls::stream<ap_uint<DATAWIDTH> >& inStream,
                      hls::stream<bool>& inStreamEoS) {
    /**
     * @brief This module reads N-bit data from stream based on
     * end of stream and writes the data to DDR. N is template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH Width of the input data stream
     *
     * @param out output memory address
     * @param inStream input hls stream
     * @param inStreamEoS input end of stream
     */

    uint32_t i = 0;
    bool eosFlag = inStreamEoS.read();
s2mm_simple:
    for (; eosFlag == false; eosFlag = inStreamEoS.read(), i++) {
#pragma HLS PIPELINE II = 1
        out[i] = inStream.read();
    }

    ap_uint<DATAWIDTH> dummy = inStream.read();
}