void checksum32(hls::stream<ap_uint<32> >& checksumInitStrm,
                hls::stream<ap_uint<8 * W> >& inStrm,
                hls::stream<ap_uint<5> >& inLenStrm,
                hls::stream<ap_uint<32> >& outStrm,
                hls::stream<ap_uint<2> >& checksumTypeStrm) {
    // Internal EOS Streams
    hls::stream<bool> endInStrm;
    hls::stream<bool> endOutStrm;
#pragma HLS STREAM variable = endInStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4

checksum_loop:
    for (ap_uint<2> checksumType = checksumTypeStrm.read(); checksumType != 3; checksumType = checksumTypeStrm.read()) {
        endInStrm << false;
        endInStrm << true;
        // CRC
        if (checksumType == 1) {
            xf::security::crc32<W>(checksumInitStrm, inStrm, inLenStrm, endInStrm, outStrm, endOutStrm);
        }
        // ADLER
        else {
            xf::security::adler32<W>(checksumInitStrm, inStrm, inLenStrm, endInStrm, outStrm, endOutStrm);
        }
        endOutStrm.read();
        endOutStrm.read();
    }
}

// Content of called function
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

    for (int i = 0; i < NUMCORES; i++) {
        endInPackLenStrm << false;
        endInPackLenStrm << true;
        adlerStrm << 1;
        xf::security::adler32<W>(adlerStrm, inStrm, inPackLenStrm, endInPackLenStrm, outStrm, endOutStrm);
        endOutStrm.read();
        endOutStrm.read();
    }
}

// Content of called function
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