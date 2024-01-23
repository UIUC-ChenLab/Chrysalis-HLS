void s2mmEosSimple(ap_uint<DATAWIDTH>* out,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   OUTSIZE_DT numItr,
                   OUTSIZE_DT inputSize,
                   OUTSIZE_DT blckSize) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param endOfStream stream to indicate end of data stream
     * @param numItr No. of iterations
     * @param inputSize Input File Size
     * @param blckSize Block Size
     */
    OUTSIZE_DT blckNum = (inputSize - 1) / blckSize + 1;
    OUTSIZE_DT idx = 0, idxCntr = 0;
    OUTSIZE_DT blckFactor = blckSize * 8 * 2 / DATAWIDTH;
    ap_uint<DATAWIDTH> dummy = 0;
    for (auto z = 0; z < (numItr - 1) * blckNum; z++) {
        bool eos = false;
    dummyLoop:
        while (!eos) {
#pragma HLS PIPELINE II = 1
            inStream.read();
            eos = endOfStream.read();
        }
    }
    for (auto z = 0; z < blckNum; z++) {
        bool eos = false;
    s2mm_eos_simple:
        for (int j = 0; eos == false; j += BURST_SIZE) {
        s2mm_eos_inner:
            for (int i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
                ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
                bool eos_tmp = (eos == true) ? true : endOfStream.read();
                out[idxCntr + j + i] = tmp;
                eos = eos_tmp;
            }
        }
        idxCntr += blckFactor;
    }
}