void s2mmSimple(ap_uint<DATAWIDTH>* out, hls::stream<ap_uint<DATAWIDTH> >& inStream, uint32_t output_size) {
    /**
     * @brief This module reads N-bit data from stream based on
     * size stream and writes the data to DDR. N is template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH Width of the input data stream
     *
     * @param out output memory address
     * @param inStream input hls stream
     * @param output_size output data size
     */

    uint8_t factor = DATAWIDTH / 8;
    uint32_t itrLim = 1 + ((output_size - 1) / factor);
s2mm_simple:
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        out[i] = inStream.read();
    }
}