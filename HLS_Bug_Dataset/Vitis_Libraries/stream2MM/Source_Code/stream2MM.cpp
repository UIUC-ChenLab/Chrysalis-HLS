void stream2MM(ap_uint<DATAWIDTH>* out,
               uint32_t* checksumData,
               hls::stream<ap_uint<32> >& checksumStream,
               hls::stream<ap_uint<DATAWIDTH> >& inStream,
               hls::stream<bool>& endOfStream,
               hls::stream<OUTSIZE_DT>& outSize,
               OUTSIZE_DT* output_size) {
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
     * @param outSize output data size
     */
    bool eos = false;
    ap_uint<DATAWIDTH> dummy = 0;

s2mm:
    for (int j = 0; eos == false; j += BURST_SIZE) {
        for (int i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
            bool eos_tmp = (eos == true) ? true : endOfStream.read();
            out[j + i] = tmp;
            eos = eos_tmp;
        }
    }
    output_size[0] = outSize.read();

    // write checksum value to DDR
    checksumData[0] = checksumStream.read();
}

// Content of called function
void s2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream, ap_uint<DATAWIDTH>* out, hls::stream<uint32_t>& inStreamSize) {
    const int c_byte_size = 8;
    const int c_factor = DATAWIDTH / c_byte_size;

    uint32_t outIdx = 0;
    uint32_t size = 1;
    uint32_t sizeIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
    mwr:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            out[outIdx + i] = inStream.read();
        }
        outIdx += size;
    }
}