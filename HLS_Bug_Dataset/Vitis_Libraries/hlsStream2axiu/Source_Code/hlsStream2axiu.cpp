void hlsStream2axiu(hls::stream<ap_uint<OUT_DWIDTH> >& inputStream,
                    hls::stream<bool>& inputStreamEos,
                    hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outAxiStream,
                    hls::stream<ap_axiu<32, 0, 0, 0> >& outAxiSizeStream,
                    hls::stream<uint32_t>& inputTotalCmpSizeStream) {
    for (uint32_t input_size = inputTotalCmpSizeStream.read(); input_size != 0;
         input_size = inputTotalCmpSizeStream.read()) {
        bool flag;
        do {
            ap_uint<OUT_DWIDTH> temp = inputStream.read();
            flag = inputStreamEos.read();
            ap_axiu<OUT_DWIDTH, 0, 0, 0> tOut;
            tOut.data = temp;
            tOut.last = flag;
            tOut.keep = -1;
            outAxiStream << tOut;
        } while (!flag);

        ap_axiu<32, 0, 0, 0> totalSize;
        totalSize.data = input_size;
        outAxiSizeStream << totalSize;
    }
}