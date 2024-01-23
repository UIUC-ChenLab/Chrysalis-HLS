void writeAccLiteralDataLL(hls::stream<ap_uint<17> >& byteStream,
                           hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream,
                           uint16_t* decSize) {
    const uint8_t c_outputByte = OUT_BYTES;
    const int c_streamWidth = c_outputByte * 8;
    ap_uint<c_streamWidth + 16> outBuffer;
    ap_uint<4> writeIdx = 0;
    uint8_t i = 0;
    uint32_t outBytes = 0;
    IntVectorStream_dt<OUT_BYTES * 8, 1> outValue;

writeAccLiterals:
    for (ap_uint<17> inData = byteStream.read(); inData.range(16, 16) == 1; inData = byteStream.read()) {
#pragma HLS PIPELINE II = 1
        outBytes += 2;
        outBuffer.range((writeIdx + 1) * 8 - 1, writeIdx * 8) = inData.range(7, 0);
        outBuffer.range((writeIdx + 2) * 8 - 1, (writeIdx + 1) * 8) = inData.range(15, 8);

        if (outBytes <= decSize[i]) {
            writeIdx += 2;
        } else {
            writeIdx += 1;
        }

        if (outBytes >= decSize[i]) {
            i += 1;
            outBytes = 0;
        }

        if (writeIdx >= c_outputByte) {
            outValue.data[0] = outBuffer;
            outValue.strobe = 1;
            literalStream << outValue;
            outBuffer >>= c_streamWidth;
            writeIdx -= c_outputByte;
        }
    }

    if (writeIdx) {
        outValue.data[0] = outBuffer;
        outValue.strobe = 1;
        literalStream << outValue;
    }
    outValue.strobe = 0;
    literalStream << outValue;
}