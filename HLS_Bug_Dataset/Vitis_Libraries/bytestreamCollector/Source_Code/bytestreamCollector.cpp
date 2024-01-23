void bytestreamCollector(hls::stream<META_DT>& lzMetaStream,
                         hls::stream<ap_uint<16> >& hfLitMetaStream,
                         hls::stream<IntVectorStream_dt<8, HFBYTES> >& hfLitBitstream,
                         hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream,
                         hls::stream<IntVectorStream_dt<8, 2> >& litEncodedStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& seqEncodedStream,
                         hls::stream<META_DT>& seqEncSizeStream,
                         hls::stream<META_DT>& bscMetaStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& outStream) {
    // Collect encoded literals and sequences data and send ordered data to output
    uint8_t fseHdrBuf[72];
#pragma HLS ARRAY_RESHAPE variable = fseHdrBuf type = cyclic factor = 2 dim = 1
#pragma HLS BIND_STORAGE variable = fseHdrBuf type = ram_2p impl = lutram
//#pragma HLS ARRAY_PARTITION variable = fseHdrBuf complete
// int rc = 0;
bsCol_main:
    while (true) {
        IntVectorStream_dt<8, DBYTES> outVal;
        // First write the FSE headers in order litblen->litlen->offset->matlen
        bool readFseHdr = false;
        auto fhVal = fseHeaderStream.read();
        if (fhVal.strobe == 0) break;
        // read meta data from LZ Compress, keep seqCnt and send rest to packer
        uint16_t seqCnt = 0;
    lz_meta_collect:
        for (uint8_t i = 0; i < 2; ++i) {
            auto lzMeta = lzMetaStream.read();
            bscMetaStream << lzMeta;
            seqCnt = lzMeta; // keeps the last value, that is seqCnt
        }
        uint16_t hdrBsLen[3] = {0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = hdrBsLen complete
        uint8_t fseHIdx = 0;
    // Buffer fse header bitstreams for litlen and offset
    buff_fse_header_bs:
        for (uint8_t i = 0; i < 3 && seqCnt > 0; ++i) {
            if (readFseHdr) fhVal = fseHeaderStream.read();
            readFseHdr = true;
            readfseS2Bram(fseHdrBuf, fseHIdx, hdrBsLen[i], fhVal, fseHeaderStream);
        }
        uint16_t hdrBsSize = 0;
        // Send FSE header for literal header
        if (readFseHdr) fhVal = fseHeaderStream.read();
    // readFseHdr = true;
    send_lit_fse_header:
        for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPEINE II = 1
            outVal.data[0] = fhVal.data[0];
            outVal.data[1] = fhVal.data[1];
            hdrBsSize += fhVal.strobe;
            outVal.strobe = fhVal.strobe;
            outStream << outVal;
        }
        // send size of literal codes fse header
        bscMetaStream << hdrBsSize;

        // Buffer fse header bitstreams for matlen
        // fhVal.strobe = 0;
        // if (seqCnt > 0) fhVal = fseHeaderStream.read();
        // readfseS2Bram(fseHdrBuf, fseHIdx, hdrBsLen[2], fhVal, fseHeaderStream);

        // Send FSE encoded bitstream for literal header
        uint8_t litEncSize = 0;
    send_lit_fse_bitstream:
        for (fhVal = litEncodedStream.read(); fhVal.strobe > 0; fhVal = litEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = fhVal.data[0];
            outVal.data[1] = fhVal.data[1];
            litEncSize += fhVal.strobe;
            outVal.strobe = fhVal.strobe;
            outStream << outVal;
        }
        // send size of literal codes fse bitstream
        bscMetaStream << (uint16_t)litEncSize;
        // send huffman encoded bitstreams for literals
        uint8_t litStreamCnt = hfLitMetaStream.read();
        // write huffman bitstream sizes to packer meta
        bscMetaStream << (uint16_t)litStreamCnt;
    read_huf_strm_sizes:
        for (uint8_t i = 0; i < litStreamCnt; ++i) { // compressed sizes
            auto _cmpS = hfLitMetaStream.read();
            bscMetaStream << _cmpS;
        }
        // Send sizes of FSE headers for litlen, offsets and matlen
        bscMetaStream << hdrBsLen[0]; // litlen
        bscMetaStream << hdrBsLen[1]; // offset
        bscMetaStream << hdrBsLen[2]; // matlen
        // read and send size of sequences fse bitstream
        auto seqBsSize = ((seqCnt > 0) ? seqEncSizeStream.read() : (META_DT)0);
        bscMetaStream << seqBsSize;
    // All metadata sent now
    // send huffman bitstreams
    send_all_hf_bitstreams:
        for (uint8_t i = 0; i < litStreamCnt; ++i) {
            IntVectorStream_dt<8, HFBYTES> hfLitVal;
        //++rc;
        send_huf_lit_bitstream:
            for (hfLitVal = hfLitBitstream.read(); hfLitVal.strobe > 0; hfLitVal = hfLitBitstream.read()) {
#pragma HLS PIPELINE II = 1
                for (uint8_t k = 0; k < HFBYTES; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = hfLitVal.data[k];
                }
                outVal.strobe = hfLitVal.strobe;
                outStream << outVal;
            }
        }
    // Send FSE header for litlen, offsets and matlen from buffer
    send_llof_fse_header_bs:
        for (uint8_t i = 0; i < fseHIdx; i += 2) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = fseHdrBuf[i];
            outVal.data[1] = fseHdrBuf[i + 1];
            outVal.strobe = ((i < fseHIdx - 2) ? 2 : fseHIdx - i);
            outStream << outVal;
        }
        // send sequences bitstream
        outVal.strobe = 0;
        if (seqCnt > 0) outVal = seqEncodedStream.read();
    send_seq_fse_bitstream:
        for (; outVal.strobe > 0; outVal = seqEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        // end of block
        outVal.strobe = 0;
        outStream << outVal;
    }
    // dump end of data from remaining input streams
    hfLitBitstream.read();
    litEncodedStream.read();
    seqEncodedStream.read();
}

// Content of called function
void readfseS2Bram(uint8_t* fseHdrBuf,
                   uint8_t& fseHIdx,
                   uint16_t& hdrBsLen,
                   IntVectorStream_dt<8, 2>& fhVal,
                   hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream) {
#pragma HLS inline off
buffer_llofml_fsebs: // taking 1.3K LUTs
    for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
        fseHdrBuf[fseHIdx] = fhVal.data[0];
        fseHdrBuf[fseHIdx + 1] = fhVal.data[1];
        fseHIdx += fhVal.strobe;
        hdrBsLen += fhVal.strobe;
    }
}