void kStreamWrite(hls::stream<ap_axiu<DATAWIDTH, 0, 0, 0> >& outKStream,
                  hls::stream<ap_uint<DATAWIDTH> >& outDataStream,
                  hls::stream<bool>& byteEos,
                  hls::stream<uint32_t>& dataSize) {
    /**
     * @brief Read N-bit wide data from internal hls streams and write to axi
     *        kernel stream till the end of stream. N is the template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH    data width of the kernel stream
     *
     * @param outKStream    output kernel stream
     * @param outDataStream output data stream from internal modules
     * @param byteEos       internal stream which indicates end of data stream
     * @param dataSize      size of data in streams
     *
     */
    uint32_t outSize = 0;
    bool lastByte = false;
    ap_uint<DATAWIDTH> tmp;
    ap_axiu<DATAWIDTH, 0, 0, 0> t1;

    do {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        lastByte = byteEos.read();
        t1.data = tmp;
        t1.last = lastByte;
        outKStream.write(t1);
    } while (!lastByte);
    outSize = dataSize.read();
}