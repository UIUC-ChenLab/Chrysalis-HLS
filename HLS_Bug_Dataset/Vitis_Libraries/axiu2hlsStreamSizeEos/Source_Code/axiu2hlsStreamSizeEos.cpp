void axiu2hlsStreamSizeEos(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inputAxiStream,
                           hls::stream<ap_axiu<32, 0, 0, 0> >& inSizeStream,
                           hls::stream<ap_uint<IN_DWIDTH> >& outputStream,
                           hls::stream<bool>& outputStreamEos,
                           hls::stream<uint32_t>& outputSizeStream) {
    const int c_parallelByte = IN_DWIDTH / 8;
    for (ap_axiu<32, 0, 0, 0> tempSize = inSizeStream.read(); tempSize.data != 0; tempSize = inSizeStream.read()) {
        uint32_t inputSize = tempSize.data;
        outputSizeStream << inputSize;
        for (uint32_t j = 0; j < inputSize; j += c_parallelByte) {
#pragma HLS PIPELINE II = 1
            ap_axiu<IN_DWIDTH, 0, 0, 0> tempVal = inputAxiStream.read();
            ap_uint<IN_DWIDTH> tmpOut = tempVal.data;
            outputStream << tmpOut;
            outputStreamEos << false;
        }
    }
    outputSizeStream << 0;
    outputStreamEos << true;
}