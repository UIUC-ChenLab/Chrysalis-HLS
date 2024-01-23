void sendHuffData(hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> >& hfcodeInStream,
                  hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hfcodeOutStream) {
    // run until strobe 0
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 3> inHfVal;
    DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> outHufVal;
    ap_uint<3> cnt = 0, idx = 0;

    outHufVal.strobe = 1;
    inHfVal.strobe = 1;
send_all_hf_data:
    while (inHfVal.strobe > 0) {
#pragma HLS PIPELINE II = 1
        if (cnt > 0) {
            outHufVal.data[0] = inHfVal.data[idx++];
            --cnt;
        } else {
            inHfVal = hfcodeInStream.read();
            cnt = inHfVal.strobe - 1;
            idx = 1;
            outHufVal.data[0] = inHfVal.data[0];
        }
        if (inHfVal.strobe > 0) hfcodeOutStream << outHufVal;
    }
    if (SEND_EOS > 0) {
        // end of huffman tree data for all blocks
        outHufVal.strobe = 0;
        hfcodeOutStream << outHufVal;
    }
}