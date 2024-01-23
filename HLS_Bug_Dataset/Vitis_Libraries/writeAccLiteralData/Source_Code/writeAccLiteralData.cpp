void writeAccLiteralData(hls::stream<ap_uint<9> >& byteStream,
                         hls::stream<IntVectorStream_dt<OUT_BYTES * 8, 1> >& literalStream) {
    ap_uint<OUT_BYTES * 8> outBuffer;
    IntVectorStream_dt<OUT_BYTES * 8, 1> outValue;
    ap_uint<4> writeIdx = 0;

huffLitUpsizer:
    for (ap_uint<9> inData = byteStream.read(); inData.range(8, 8) == 1; inData = byteStream.read()) {
#pragma HLS PIPELINE II = 1
        outBuffer.range((writeIdx + 1) * 8 - 1, writeIdx * 8) = inData.range(7, 0);
        writeIdx++;
        if (writeIdx == OUT_BYTES) {
            outValue.data[0] = outBuffer;
            outValue.strobe = 1;
            literalStream << outValue;
            writeIdx = 0;
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