void s2mm32(hls::stream<ap_uint<32> >& inStream, hls::stream<bool>& inStreamEos, ap_uint<32>* outChecksumData) {
    bool eos = inStreamEos.read();
    if (!eos) outChecksumData[0] = inStream.read();
    inStreamEos.read();
}