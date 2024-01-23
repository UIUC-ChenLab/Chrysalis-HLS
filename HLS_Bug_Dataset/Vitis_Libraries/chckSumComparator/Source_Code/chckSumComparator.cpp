void chckSumComparator(hls::stream<ap_uint<32> >& checkSum1,
                       hls::stream<ap_uint<32> >& checkSum2,
                       hls::stream<bool>& output) {
    auto chk1 = checkSum1.read();
    auto chk2 = checkSum2.read();
    output << (chk1 != chk2);
}