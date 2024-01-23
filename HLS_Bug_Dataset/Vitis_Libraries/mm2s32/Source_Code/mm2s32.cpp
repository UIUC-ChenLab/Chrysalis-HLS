void mm2s32(const ap_uint<PARALLEL_BYTE * 8>* in,
            const ap_uint<32>* inChecksumData,
            hls::stream<ap_uint<PARALLEL_BYTE * 8> >& outStream,
            hls::stream<ap_uint<32> >& outChecksumStream,
            hls::stream<ap_uint<32> >& outLenStream,
            hls::stream<bool>& outLenStreamEos,
            ap_uint<32> inputSize) {
    uint32_t inSize_gmemwidth = (inputSize - 1) / PARALLEL_BYTE + 1;
    uint32_t readSize = 0;

    outLenStream << inputSize;
    outChecksumStream << inChecksumData[0];
    outLenStreamEos << false;
    outLenStreamEos << true;

mm2s_simple:
    for (uint32_t i = 0; i < inSize_gmemwidth; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<PARALLEL_BYTE* 8> temp = in[i];
        outStream << temp;
    }
}