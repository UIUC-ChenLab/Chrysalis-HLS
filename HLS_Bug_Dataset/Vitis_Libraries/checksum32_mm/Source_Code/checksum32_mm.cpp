void checksum32_mm(const ap_uint<PARALLEL_BYTE * 8>* in,
                   ap_uint<32>* checksumData,
                   ap_uint<32> inputSize,
                   const bool checksumType) {
#pragma HLS dataflow

    hls::stream<ap_uint<PARALLEL_BYTE * 8> > inStream;
    hls::stream<ap_uint<32> > inChecksumStream;
    hls::stream<ap_uint<32> > inLenStream;
    hls::stream<ap_uint<32> > outStream;
    hls::stream<bool> inLenStreamEos;
    hls::stream<bool> outStreamEos;

#pragma HLS STREAM variable = inStream depth = 32

    // mm2s
    details::mm2s32<PARALLEL_BYTE>(in, checksumData, inStream, inChecksumStream, inLenStream, inLenStreamEos,
                                   inputSize);

    // Checksum32
    checksum32<PARALLEL_BYTE>(inChecksumStream, inStream, inLenStream, inLenStreamEos, outStream, outStreamEos,
                              checksumType);

    // s2mm
    details::s2mm32(outStream, outStreamEos, checksumData);
}

// Content of called function
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

// Content of called function
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

// Content of called function
void s2mm32(hls::stream<ap_uint<32> >& inStream, hls::stream<bool>& inStreamEos, ap_uint<32>* outChecksumData) {
    bool eos = inStreamEos.read();
    if (!eos) outChecksumData[0] = inStream.read();
    inStreamEos.read();
}