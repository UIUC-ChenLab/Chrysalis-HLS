void kStreamWrite(hls::stream<ap_axiu<DATAWIDTH, 0, 0, 0> >& outKStream,
                  hls::stream<ap_axiu<AXIWIDTH, 0, 0, 0> >& outKSizeStream,
                  hls::stream<ap_uint<DATAWIDTH> >& inDataStream,
                  hls::stream<bool>& inDataStreamEoS,
                  hls::stream<SIZE_DT>& inSizeStream) {
    /**
     * @brief Read N-bit wide data from internal hls streams and write to axi
     *        kernel stream for the given size. N is the template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH    data width of the kernel stream
     *
     * @param outKStream    output kernel stream
     * @param outDataStream output data stream from internal modules
     * @param dataSize      size of data in streams
     *
     */
    // read the original size
    ap_uint<DATAWIDTH> inValue;
    ap_axiu<DATAWIDTH, 0, 0, 0> outValue;
    ap_axiu<AXIWIDTH, 0, 0, 0> decompressSize;

    bool eosFlag = inDataStreamEoS.read();
    inValue = inDataStream.read();
    outValue.data = inValue;
    outValue.last = false;
    outValue.keep = -1;

    while (!eosFlag) {
#pragma HLS PIPELINE II = 1
        eosFlag = inDataStreamEoS.read();
        if (eosFlag) {
            decompressSize.data = inSizeStream.read();
            decompressSize.last = true;
            outKSizeStream.write(decompressSize);

            uint8_t value = decompressSize.data % (DATAWIDTH / 8);
            outValue.last = true;
            outValue.keep = ((1 << value) - 1);
        }

        outKStream.write(outValue);

        inValue = inDataStream.read();
        outValue.data = inValue;
        outValue.last = false;
        outValue.keep = -1;
    }
}