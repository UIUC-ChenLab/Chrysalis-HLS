void s2mmEosSimple(ap_uint<DATAWIDTH>* out,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   uint32_t numItr,
                   hls::stream<OUTSIZE_DT>& outSize,
                   OUTSIZE_DT* output_size) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @param out output memory address
     * @param inStream input stream
     * @param endOfStream stream to indicate end of data stream
     * @param outSize output data size
     * @param output_size output size memory address
     */
    ap_uint<DATAWIDTH> dummy = 0;
    for (auto z = 0; z < numItr; z++) {
        bool eos = false;
    s2mm_eos_outer:
        for (int j = 0; eos == false; j += BURST_SIZE) {
        s2mm_eos_inner:
            for (int i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
                ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
                bool eos_tmp = (eos == true) ? true : endOfStream.read();
                out[j + i] = tmp;
                eos = eos_tmp;
            }
        }
        output_size[0] = outSize.read();
        if (TUSR_DWIDTH != 0) output_size[1] = outSize.read();
    }
}