void adler32(hls::stream<ap_uint<W * 8> >& inStrm,
             hls::stream<ap_uint<5> >& inPackLenStrm,
             hls::stream<ap_uint<32> >& outStrm) {
    // To be deprecated
    hls::stream<bool> endInPackLenStrm;
    hls::stream<bool> endOutStrm;
    hls::stream<ap_uint<32> > adlerStrm;
#pragma HLS STREAM variable = endInPackLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4
#pragma HLS STREAM variable = adlerStrm depth = 4
    endInPackLenStrm << false;
    endInPackLenStrm << true;
    adlerStrm << 1;
    xf::security::adler32<W>(adlerStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
    endOutStrm.read();
    endOutStrm.read();
}