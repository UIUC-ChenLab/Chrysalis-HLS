void packCompressedFrame(hls::stream<IntVectorStream_dt<16, 1> >& packerMetaStream,
                         hls::stream<META_DT>& bscMetaStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& bscBitstream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& outStream) {
    // Collect encoded literals and sequences data and send formatted data to output
    constexpr bool isSingleSegment = 0; // set if using 1 block/frame
    bool blockRead = false;
    IntVectorStream_dt<8, DBYTES> outVal;
    ap_uint<8> frameHeaderBuf[14];
#pragma HLS ARRAY_PARTITION variable = frameHeaderBuf complete
    uint8_t fhIdx = 0;
    uint8_t fhdFixedOff = 6;
    uint32_t outSize = 0;

    /* Frame format
     * <Magic_Number><Frame_Header><Data_Block(s).....><Content_Checksum>
     * 	  4 bytes      2-14 bytes      n bytes....          0-4 bytes
     */
    /* Frame_Header format
     * <Frame_Header_Descriptor><Window_Descriptor><Dictionary_Id><Frame_Content_Size>
     * 		    1 byte				  0-1 byte		  0-4 bytes         0-8 bytes
     */
    // Add Magic_Number
    frameHeaderBuf[0] = xf::compression::details::c_magicNumber.range(7, 0);
    frameHeaderBuf[1] = xf::compression::details::c_magicNumber.range(15, 8);
    frameHeaderBuf[2] = xf::compression::details::c_magicNumber.range(23, 16);
    frameHeaderBuf[3] = xf::compression::details::c_magicNumber.range(31, 24);
    // Add frame header
    ap_uint<8> frameHeader = 0;
    uint8_t windowLog = bitsUsed31((uint32_t)BLOCK_SIZE);
    uint32_t windowBase = (uint32_t)1 << windowLog;
    frameHeaderBuf[5].range(7, 3) = windowLog - 10;
    frameHeaderBuf[5].range(2, 0) = (uint8_t)((8 * (BLOCK_SIZE - windowBase)) >> windowLog);
zstd_frame_packer:
    while (true) {
        int outSize = 0;
        fhIdx = fhdFixedOff;
        // Read all the metadata needed to write frame header
        // read block meta data
        auto meta = packerMetaStream.read();
        if (meta.strobe == 0) break;
        /*** Start: Frame Content Size --- Valid ONLY with 1 block/frame case***/
        // In regular case, it should be sum of sizes of input blocks in a frame
        // block size can be max 128KB, using 3 bytes, if less, then 2nd read will be 0, with highest bit indicating
        // last data block
        ap_uint<24> fcs = 0;
        fcs.range(15, 0) = meta.data[0]; // write 16-bit part of size
        meta = packerMetaStream.read();
        fcs.range(23, 16) = (uint8_t)meta.data[0]; // remaining 8-bit of size
        // write fcs as block size for raw block case, to output
        // fill output buffer
        outVal.strobe = 4;
        outVal.data[0] = fcs.range(7, 0);
        outVal.data[1] = fcs.range(15, 8);
        outVal.data[2] = fcs.range(23, 16);
        outVal.data[3] = 0;
        outStream << outVal;

        if (fcs > 255) {
            uint8_t c_fcsFlag = ((fcs < (64 * 1024)) ? 1 : 2);
            // write full fcs (to read as per fhIdx
            if (c_fcsFlag == 1) fcs -= 256;
            frameHeaderBuf[6] = fcs.range(7, 0);
            frameHeaderBuf[7] = fcs.range(15, 8);
            frameHeaderBuf[8] = fcs.range(23, 16);
            frameHeaderBuf[9] = 0;
            // set increment
            fhIdx += (1 << c_fcsFlag);
            // we have 1 block/frame, therefore single segment flag can be set
            // so that window size becomes equal to frame content size
            frameHeader.range(7, 6) = c_fcsFlag;
        } else {
            // do not write frame content size to header
        }
        frameHeaderBuf[4] = frameHeader;
    /*** End: Frame Content Size ***/
    // Now write frame header, pre-fixed with relevant meta data
    // Write the size of frame header and block size, this is not part of the format, it is for use in next module
    // Write Frame header
    send_frame_header:
        for (uint8_t i = 0; i < fhIdx; i += DBYTES) {
#pragma HLS PIPELINE II = 1
            for (uint8_t k = 0; k < DBYTES; ++k) {
#pragma HLS UNROLL
                outVal.data[k] = (i + k < fhIdx) ? frameHeaderBuf[i + k] : (ap_uint<8>)0;
            }
            outVal.strobe = ((i < fhIdx - DBYTES) ? DBYTES : (fhIdx - i));
            outStream << outVal;
            outSize += outVal.strobe;
        }
        // end of header
        outVal.strobe = 0;
        outStream << outVal;
        // skip stored block
        if (fcs < MIN_BLK_SIZE + 1) { // *** MIN_BLK_SIZE < 256 COMPULSORY ***
            // send strobe 0, to indicate end of block for frame data
            outVal.strobe = 0;
            outStream << outVal;
            continue;
        }

        // Read all block meta data and add 3-byte block header
        auto litCnt = bscMetaStream.read();
        auto seqCnt = bscMetaStream.read();

        ap_uint<40> litSecHdr = 0;
        ap_uint<32> seqSecHdr = 0; //<1-3 bytes seq cnt><1 byte symCompMode>
        uint32_t blockSize = 0;
        ap_uint<16> streamSizes[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSizes complete
        uint8_t hfStreamCnt = 0;
        uint16_t hfStrmSize = 0;
    read_meta_sizes:
        for (uint8_t i = 0; i < 7 + hfStreamCnt; ++i) {
#pragma HLS PIPELINE II = 1
            auto hdrSzVal = bscMetaStream.read();
            streamSizes[i] = hdrSzVal;
            if (i == 2) {
                hfStreamCnt = hdrSzVal; // 3rd value is number of literal streams
            } else {
                if (i > 2 + hfStreamCnt) blockSize += hdrSzVal; // add the size of compressed sequence bitstreams
                if (i > 2 && i - 3 < hfStreamCnt) {
                    hfStrmSize += hdrSzVal; // add huffman stream sizes
                }
            }
        }

        /*	Header Memory format
         * <-Block Header-><---------------- Literal Section -------------><------------ Sequences Section ------------>
         * 				   <Section_Header><HF Header><FSE Header, Bitstream><SeqCnt><SymCompMode><FSE
         * Tables
         * LL-OF-ML>
         * 					   5 bytes		  1 byte	 	128 bytes       1-3bytes
         * 1
         * byte
         * 0-63 bytes
         * 	  3 bytes							134 byte                          Upto
         * 67
         * bytes
         */
        // set sequence section header
        uint8_t seqBytes = 1;
        if (seqCnt < 128) {
            seqSecHdr.range(7, 0) = seqCnt; // use 1 byte only
        } else if (seqCnt > 127 && seqCnt < 32512) {
            seqBytes = 2;
            // calculate Number_of_Sequences = ((byte0-128) << 8) + byte1
            seqSecHdr.range(15, 8) = (uint8_t)seqCnt;
            seqSecHdr.range(7, 0) = (uint8_t)((seqCnt >> 8) & 0x0000FF) + 128;
        } else {
            seqBytes = 3;
            seqSecHdr.range(7, 0) = 255;
            // calculate Number_of_Sequences = byte1 + (byte2<<8) + 0x7F00
            seqSecHdr.range(23, 8) = seqCnt - 32512;
        }
        // ll,of,ml compression modes
        if (seqCnt > 0) {
            seqSecHdr.range((8 * (seqBytes + 1)) - 1, (8 * seqBytes)) = (uint8_t)((2 << 6) + (2 << 4) + (2 << 2));
            ++seqBytes;
        }
        // set literal section header
        uint8_t litSecHdrBCnt = 3 + (2 * (uint8_t)(hfStreamCnt > 1));
        uint8_t lsSzBits = 10 + (8 * (uint8_t)(hfStreamCnt > 1));
        litSecHdr.range(1, 0) = 2;                         // Block type -> Compressed
        litSecHdr.range(3, 2) = (hfStreamCnt > 1) ? 3 : 0; // Size format
        // regenerated size is litCnt
        litSecHdr.range(3 + lsSzBits, 4) = (uint32_t)litCnt; // original size
        // compressed size--> <HF tree Desc:<1byte HF header><fseBSHeader><fseBitstream>><0-6bytes jumpTable><HF Streams
        // <1><2><3><4>>
        litSecHdr.range(3 + (2 * lsSzBits), 4 + lsSzBits) =
            (uint32_t)(1 + streamSizes[0] + streamSizes[1] + (6 * (uint8_t)(hfStreamCnt > 1)) + hfStrmSize);
        // calculate block size, by adding bytes needed by different headers
        // sequence fse bitstreams size already added to blockSize
        blockSize += (litSecHdrBCnt + (uint32_t)litSecHdr.range(3 + (2 * lsSzBits), 4 + lsSzBits)) + seqBytes;
        // Write Block header
        /*
         * Block_Header: 3 bytes
         * <Last_Block><Block_Type><Block_Size>
         *    bit 0		 bits 1-2    bits 3-23
         */
        // Block Type = 2 (Compressed Block), 1 always last block in frame
        ap_uint<24> blockHeader = (uint32_t)1 + (2 << 1);
        blockHeader.range(23, 3) = blockSize;

        // Write Block Header -> Block Content
        outVal.data[0] = blockHeader.range(7, 0);
        outVal.data[1] = blockHeader.range(15, 8);
        outVal.data[2] = blockHeader.range(23, 16);
        outVal.data[3] = 0;
        outVal.strobe = 3;
        outStream << outVal;
        outSize += outVal.strobe;
        // Write Block Content
        // Write Literal Section header
        if (DBYTES < 5) {
        write_lit_sec_hdr:
            for (uint8_t i = 0; i < litSecHdrBCnt; i += DBYTES) {
#pragma HLS PIPELINE II = 1
                for (uint8_t k = 0; k < DBYTES; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = litSecHdr.range(7 + (k * 8), k * 8);
                }
                outVal.strobe = ((i < litSecHdrBCnt - DBYTES) ? DBYTES : (litSecHdrBCnt - i));
                outStream << outVal;
                litSecHdr >>= (DBYTES * 8);
                outSize += outVal.strobe;
            }
        } else {
            for (uint8_t k = 0; k < 5; ++k) { // litSecHdr is 40-bits => 5 bytes
#pragma HLS UNROLL
                outVal.data[k] = litSecHdr.range(7 + (k * 8), k * 8);
            }
            outVal.strobe = litSecHdrBCnt;
            outStream << outVal;
            outSize += litSecHdrBCnt;
        }
        // Write fse header
        outVal.data[0] = (uint8_t)(streamSizes[0] + streamSizes[1]);
        outVal.strobe = 1;
        outStream << outVal;
        outSize += outVal.strobe;
    // read literal fse header, bitstream and literal huffman bitstreams
    write_lit_sec:
        for (uint8_t k = 0; k < 3; ++k) {
            uint32_t itrSize = ((k < 2) ? (uint32_t)streamSizes[k] : (uint32_t)hfStrmSize);
        write_lithd_fse_data:
            for (uint16_t i = 0; i < itrSize; i += outVal.strobe) {
#pragma HLS PIPELINE II = 1
                outVal = bscBitstream.read();
                outStream << outVal;
                outSize += outVal.strobe;
            }
            // Write jump table and huffman streams
            if (k == 1 && hfStreamCnt > 1) {
                outVal.data[0] = streamSizes[3].range(7, 0);
                outVal.data[1] = streamSizes[3].range(15, 8);
                outVal.data[2] = streamSizes[4].range(7, 0);
                outVal.data[3] = streamSizes[4].range(15, 8);
                if (DBYTES < 8) {
                    outVal.strobe = 4;
                    outStream << outVal;
                    outSize += outVal.strobe;
                    outVal.data[0] = streamSizes[5].range(7, 0);
                    outVal.data[1] = streamSizes[5].range(15, 8);
                    outVal.strobe = 2;
                    outStream << outVal;
                } else {
                    outVal.data[4] = streamSizes[5].range(7, 0);
                    outVal.data[5] = streamSizes[5].range(15, 8);
                    outVal.strobe = 6;
                    outStream << outVal;
                }
                outSize += outVal.strobe;
            }
        }
        // write sequence section header
        // seqSecHdr can be max 32-bits and at least 8-bits
        outVal.data[0] = seqSecHdr.range(7, 0);
        outVal.data[1] = seqSecHdr.range(15, 8);
        outVal.data[2] = seqSecHdr.range(23, 16);
        outVal.data[3] = seqSecHdr.range(31, 24);
        outVal.strobe = seqBytes;
        outStream << outVal;
        outSize += outVal.strobe;
    // write sequence headers and bitstreams(entire remaining 16-bit and 32-bit input bitstreams)
    send_seq_fse_hdrs:
        for (outVal = bscBitstream.read(); outVal.strobe > 0; outVal = bscBitstream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
            outSize += outVal.strobe;
        }
        // send strobe 0
        outVal.strobe = 0;
        outStream << outVal;
        // printf("CMP out size: %d\n", outSize);
    }
}