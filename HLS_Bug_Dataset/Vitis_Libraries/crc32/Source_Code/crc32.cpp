void crc32(hls::stream<ap_uint<32> >& crcInitStrm,
           hls::stream<ap_uint<8 * W> >& inStrm,
           hls::stream<ap_uint<5> >& inPackLenStrm,
           hls::stream<ap_uint<32> >& outStrm) {
    hls::stream<bool> endInPackLenStrm;
    hls::stream<bool> endOutStrm;
#pragma HLS STREAM variable = endInPackLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4
    endInPackLenStrm << false;
    endInPackLenStrm << true;
    xf::security::crc32<W>(crcInitStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
    endOutStrm.read();
    endOutStrm.read();
}