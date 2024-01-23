void streamDataK2dm(hls::stream<ap_uint<STREAMDWIDTH> >& out,
                    hls::stream<bool>& bytEos,
                    hls::stream<uint32_t>& dataSize,
                    hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& dmOutStream) {
    /**
     * @brief Read data from kernel axi stream byte by byte and write to hls stream and indicate end of stream.
     *
     * @param out           output hls stream
     * @param bytEos        internal stream which indicates end of data stream
     * @param dataSize      size of data in streams
     * @param dmOutStream   input kernel axi stream to be read
     *
     */
    // read data from decompression kernel output to global memory output
    ap_axiu<STREAMDWIDTH, 0, 0, 0> dataout;
    bool last = false;
    uint32_t outSize = 0;
    do {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        last = dataout.last;
        bytEos << last;
        out << dataout.data;
        outSize++;
    } while (!last);
    dataSize << outSize - 1; // read encoded size from decompression kernel
}