void zstdCompressStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                           hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
#pragma HLS DATAFLOW
    hls::stream<IntVectorStream_dt<IN_DWIDTH, 1> > inZstdStream("inZstdStream");
    hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> > outCompressedStream("outCompressedStream");

#pragma HLS STREAM variable = inZstdStream depth = 8
#pragma HLS STREAM variable = outCompressedStream depth = 8

    // AXI 2 HLS Stream
    xf::compression::details::zstdAxiu2hlsStream<IN_DWIDTH>(inStream, inZstdStream);

    // Zlib Compress Stream IO Engine
    xf::compression::zstdCompressCore<BLOCK_SIZE, LZWINDOW_SIZE, MIN_BLCK_SIZE>(inZstdStream, outCompressedStream);

    // HLS 2 AXI Stream
    xf::compression::details::zstdHlsVectorStream2axiu<OUT_DWIDTH>(outCompressedStream, outStream);
}

// Content of called function
void zstdCompressCore(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // zstd compression main module
    constexpr uint32_t c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr uint32_t c_dataUpSDepth = BLOCK_SIZE / 8;
    constexpr uint32_t c_hfLitStreamDepth = BLOCK_SIZE / 2;
    constexpr uint32_t c_seqBlockDepth = BLOCK_SIZE / 8;

    // Internal streams
    hls::stream<IntVectorStream_dt<8, 1> > inBlockStream("inBlockStream");
    hls::stream<IntVectorStream_dt<8, 1> > strdBlockStream("strdBlockStream");
    hls::stream<IntVectorStream_dt<8, 4> > strdDsBlockStream("strdDsBlockStream");
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
    hls::stream<ap_uint<68> > rawUpsizedFinalStream("rawUpsizedFinalStream");
    hls::stream<ap_uint<68> > rwbUpsizedStream("rwbUpsizedStream"); // 1 URAM
#pragma HLS STREAM variable = inBlockStream depth = 16
#pragma HLS STREAM variable = strdBlockStream depth = 16
#pragma HLS STREAM variable = strdDsBlockStream depth = 16
#pragma HLS STREAM variable = packerMetaStream depth = 16
#pragma HLS STREAM variable = rawUpsizedFinalStream depth = 16
#pragma HLS STREAM variable = rwbUpsizedStream depth = c_dataUpSDepth
#pragma HLS BIND_STORAGE variable = rwbUpsizedStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = rawUpsizedFinalStream type = FIFO impl = LUTRAM

    hls::stream<IntVectorStream_dt<8, 1> > litStream("litStream");
    hls::stream<IntVectorStream_dt<8, 1> > reverseLitStream("reverseLitStream");
    hls::stream<IntVectorStream_dt<8, 1> > dszLitStream("litStream");
    hls::stream<ap_uint<68> > litUpsizedStream("litUpsizedStream"); // 1 URAM
#pragma HLS STREAM variable = litStream depth = 64
#pragma HLS STREAM variable = reverseLitStream depth = 16
#pragma HLS STREAM variable = dszLitStream depth = 8
#pragma HLS STREAM variable = litUpsizedStream depth = c_dataUpSDepth
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = URAM

    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > seqStream("seqStream");
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream(
        "reverseSeqStream"); // 1 URAM
#pragma HLS STREAM variable = seqStream depth = 32
// 4K depth needed to keep reading input sequences even if previous block decoding waits for fse table generation
#pragma HLS STREAM variable = reverseSeqStream depth = 4096

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream("litFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream("seqFreqStream");
#pragma HLS STREAM variable = litFreqStream depth = 16
#pragma HLS STREAM variable = seqFreqStream depth = 128

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
#pragma HLS STREAM variable = wghtFreqStream depth = 8
#pragma HLS STREAM variable = freqStream depth = 128

    hls::stream<ap_uint<c_freq_dwidth> > lzMetaStream("lzMetaStream");
    hls::stream<bool> rleFlagStream("rleFlagStream");
    hls::stream<ap_uint<2> > rwbFlagStream("rwbFlagStream");
    hls::stream<ap_uint<2> > rwbFinalFlagStream1("rwbFinalFlagStream1");
    hls::stream<ap_uint<2> > rwbFinalFlagStream2("rwbFinalFlagStream2");
    hls::stream<ap_uint<c_freq_dwidth> > litCntStream("litCntStream");
#pragma HLS STREAM variable = lzMetaStream depth = 16
#pragma HLS STREAM variable = rleFlagStream depth = 4
#pragma HLS STREAM variable = rwbFlagStream depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream1 depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream2 depth = 4
#pragma HLS STREAM variable = litCntStream depth = 8

    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");
#pragma HLS STREAM variable = fseHeaderStream depth = 128
#pragma HLS STREAM variable = fseLitTableStream depth = 8
#pragma HLS STREAM variable = fseSeqTableStream depth = 8

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hfEncodedLitStream;
    hls::stream<IntVectorStream_dt<8, 2> > hfLitBitstream("hfLitBitstream");
#pragma HLS STREAM variable = hufLitMetaStream depth = 128
#pragma HLS STREAM variable = hufCodeStream depth = 8
#pragma HLS STREAM variable = hufWeightStream depth = 8
#pragma HLS STREAM variable = hfEncodedLitStream depth = 256
#pragma HLS STREAM variable = hfLitBitstream depth = c_hfLitStreamDepth

    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<ap_uint<c_freq_dwidth> > seqEncSizeStream("seqEncSizeStream");
    hls::stream<ap_uint<68> > seqEncodedStream("seqEncodedStream");
    hls::stream<IntVectorStream_dt<8, 4> > seqEncodedDszStream("seqEncodedDszStream");
#pragma HLS STREAM variable = litEncodedStream depth = 128
#pragma HLS STREAM variable = seqEncSizeStream depth = 4
#pragma HLS STREAM variable = seqEncodedDszStream depth = 8
#pragma HLS STREAM variable = seqEncodedStream depth = c_seqBlockDepth
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = URAM

    hls::stream<ap_uint<16> > bscMetaStream("bscMetaStream");
    hls::stream<IntVectorStream_dt<8, 4> > bsc32Bitstream("bsc32Bitstream");
#pragma HLS STREAM variable = bscMetaStream depth = 128
#pragma HLS STREAM variable = bsc32Bitstream depth = 256

    hls::stream<IntVectorStream_dt<8, 4> > cmpFrameStream("cmpFrameStream");
    hls::stream<IntVectorStream_dt<8, 4> > cmpFrameFinalStream("cmpFrameFinalStream");
#pragma HLS STREAM variable = cmpFrameStream depth = 64
#pragma HLS STREAM variable = cmpFrameFinalStream depth = 16

#pragma HLS dataflow

    // Module-1: Input reading and LZ77 compression
    {
        details::inputDistributer<BLOCK_SIZE, MIN_BLCK_SIZE>(inStream, inBlockStream, strdBlockStream,
                                                             packerMetaStream);
        // Upsize raw data for raw block
        details::simpleStreamUpsizer<8, 64, 4>(strdBlockStream, rwbUpsizedStream);
        // LZ77 compression of input blocks to get separate streams
        // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
        details::getLitSequences<BLOCK_SIZE, c_freq_dwidth>(inBlockStream, litStream, seqStream, litFreqStream,
                                                            seqFreqStream, rleFlagStream, rwbFlagStream, lzMetaStream,
                                                            litCntStream);
        details::skipPassRawBlock<64, 4>(rwbUpsizedStream, rwbFlagStream, rawUpsizedFinalStream, rwbFinalFlagStream1,
                                         rwbFinalFlagStream2);
        // Upsize literals
        details::simpleStreamUpsizer<8, 64, 4>(litStream, litUpsizedStream);
        // Buffer in stream URAM and downsize literals
        details::bufferDownsizer<64, 8, 4>(litUpsizedStream, dszLitStream);
    }
    // Module-2: Encoding table generation and data preparation
    {
        // Buffer, reverse and break input literal stream into 4 streams of 1/4th size
        details::preProcessLitStream<BLOCK_SIZE, c_freq_dwidth, PARALLEL_LIT_STREAMS>(dszLitStream, litCntStream,
                                                                                      reverseLitStream);
        // Reverse sequences stream
        details::reverseSeq<BLOCK_SIZE, c_freq_dwidth, MIN_MATCH>(seqStream, reverseSeqStream);
        // generate hufffman tree and get codes-bitlens
        zstdTreegenStream<c_freq_dwidth, details::c_maxZstdHfBits>(litFreqStream, hufCodeStream, hufWeightStream,
                                                                   wghtFreqStream);
        // feed frequency data to fse table gen from literals and sequences
        details::frequencySequencer<c_freq_dwidth>(wghtFreqStream, seqFreqStream, freqStream);
        // generate FSE Tables for litlen, matlen, offset and literal-bitlen
        details::fseTableGen(freqStream, fseHeaderStream, fseLitTableStream, fseSeqTableStream);
    }
    // Module-3: Encoding literal and sequences
    {
        // Huffman encoding of literal stream
        details::zstdHuffmanEncoder<details::c_maxZstdHfBits>(reverseLitStream, rleFlagStream, hufCodeStream,
                                                              hfEncodedLitStream, hufLitMetaStream);
        // Huffman bitstream packer
        details::zstdHuffBitPacker<details::c_maxZstdHfBits>(hfEncodedLitStream, hfLitBitstream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, fseSeqTableStream, seqEncodedStream, seqEncSizeStream);
    }
    // Module-4: Output block and frame packing
    {
        // Buffer in stream URAM and downsize fse encoded sequence bitstream
        details::bufferDownsizerVec<64, 32, 4>(seqEncodedStream, seqEncodedDszStream);
        // collect data from different input byte streams and output 2 continuous streams
        details::bytestreamCollector<c_freq_dwidth>(lzMetaStream, hufLitMetaStream, hfLitBitstream, fseHeaderStream,
                                                    litEncodedStream, seqEncodedDszStream, seqEncSizeStream,
                                                    bscMetaStream, bsc32Bitstream);
        // pack compressed data into single sequential block stream
        details::packCompressedFrame<BLOCK_SIZE, MIN_BLCK_SIZE, c_freq_dwidth>(packerMetaStream, bscMetaStream,
                                                                               bsc32Bitstream, cmpFrameStream);
        details::skipPassCmpBlock(cmpFrameStream, rwbFinalFlagStream1, cmpFrameFinalStream);
        // Buffer in stream URAM and downsize raw block data
        details::bufferDownsizerVec<64, 32, 4>(rawUpsizedFinalStream, strdDsBlockStream);
        // Output compressed or raw block based on input flag stream
        details::streamCmpStrdFrame(strdDsBlockStream, cmpFrameFinalStream, rwbFinalFlagStream2, outStream);
    }
}

// Content of called function
void skipPassRawBlock(hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& inRawStream,
                      hls::stream<ap_uint<2> >& inStbFlagStream,
                      hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& outRawStream,
                      hls::stream<ap_uint<2> >& outStbFlagStream1,
                      hls::stream<ap_uint<2> >& outStbFlagStream2) {
    // read and dump the raw block data when not needed
    ap_uint<IN_DWIDTH + STB_WIDTH> inVal;
    bool stbFlagStrobe = 1;
    ap_uint<2> outFlagVal = 1; // <stb Flag 1-bit><strobe 1-bit>
outer_rbk_loop:
    while (true) {
        inVal = inRawStream.read();
        if (inVal.range(STB_WIDTH - 1, 0) == 0) break;
        auto inFlagVal = inStbFlagStream.read();
        bool isRawBlk = inFlagVal.range(1, 1);
        stbFlagStrobe = inFlagVal.range(0, 0);
        // if last block is present (as control reached here) and strobe for stbFlagStream is 0
        // therefore it is minimum block case, so set isRawBlk flag
        if (stbFlagStrobe == 0) isRawBlk = 1;
        outFlagVal.range(1, 1) = (ap_uint<1>)isRawBlk;
        outStbFlagStream1 << outFlagVal;
        outStbFlagStream2 << outFlagVal;
    skip_pass_raw_block_loop:
        for (; inVal.range(STB_WIDTH - 1, 0) > 0; inVal = inRawStream.read()) {
#pragma HLS PIPELINE II = 1
            if (isRawBlk) outRawStream << inVal;
        }
        // send strobe 0 for end of block
        if (isRawBlk) outRawStream << inVal;
    }
    if (stbFlagStrobe > 0) inStbFlagStream.read();
    // end of data
    inVal = 0;
    outRawStream << inVal;
    outFlagVal = 0;
    outStbFlagStream1 << outFlagVal;
    outStbFlagStream2 << outFlagVal;
}

// Content of called function
void skipPassCmpBlock(hls::stream<IntVectorStream_dt<8, DBYTES> >& inCmpStream,
                      hls::stream<ap_uint<2> >& rawBlockFlagStream,
                      hls::stream<IntVectorStream_dt<8, DBYTES> >& outCmpStream) {
    IntVectorStream_dt<8, DBYTES> outVal;
stream_skip_cmp_blk:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
    stream_cmp_frm_hdr_blk:
        for (auto i = 0; i < 2; ++i) {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpStream.read(); outVal.strobe > 0; outVal = inCmpStream.read()) {
#pragma HLS PIPELINE II = 1
                if (isRawBlk == false || i == 0) outCmpStream << outVal;
            }
            outVal.strobe = 0;
            if (isRawBlk == false || i == 0) outCmpStream << outVal;
        }
    }
}

// Content of called function
void streamCmpStrdFrame(hls::stream<IntVectorStream_dt<8, 4> >& inRawBStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& inCmpBStream,
                        hls::stream<ap_uint<2> >& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    IntVectorStream_dt<8, 4> outVal;
    ap_uint<24> strdBlockHeader = 1; // bit-0 = 1, indicating last block, bits 1-2 = 0, indicating raw block
stream_cmp_file:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
        // read frame content size
        outVal = inCmpBStream.read();
        strdBlockHeader.range(10, 3) = (uint8_t)outVal.data[0];
        strdBlockHeader.range(18, 11) = (uint8_t)outVal.data[1];
        strdBlockHeader.range(23, 19) = (uint8_t)outVal.data[2];
    // write the frame header, written for each block of input data as stated in its meta data
    write_frame_header:
        for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        if (isRawBlk) {
            // Write stored block header
            outVal.data[0] = strdBlockHeader.range(7, 0);
            outVal.data[1] = strdBlockHeader.range(15, 8);
            outVal.data[2] = strdBlockHeader.range(23, 16);
            outVal.strobe = 3;
            outStream << outVal;
        write_raw_blk_data:
            for (auto rbVal = inRawBStream.read(); rbVal.strobe > 0; rbVal = inRawBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << rbVal;
            }
        } else {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << outVal;
            }
        }
    }
    // dump last strobe 0
    inRawBStream.read();
    // end of file
    outVal.strobe = 0;
    outStream << outVal;
}

// Content of called function
void inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    // Internal streams
    hls::stream<IntVectorStream_dt<8, 1> > intmDataStream("intmDataStream");
    hls::stream<bool> rawBlockFlagStream("rawBlockFlagStream");

#pragma HLS STREAM variable = intmDataStream depth = 256
#pragma HLS STREAM variable = rawBlockFlagStream depth = 4

#pragma HLS dataflow
    xf::compression::details::inputBufferMinBlock<BLOCK_SIZE, MIN_BLK_SIZE>(inStream, rawBlockFlagStream,
                                                                            intmDataStream);

    xf::compression::details::__inputDistributer<BLOCK_SIZE>(intmDataStream, rawBlockFlagStream, outStream,
                                                             outStrdStream, blockMetaStream);
}

// Content of called function
void inputBufferMinBlock(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                         hls::stream<bool>& rawBlockFlagStream,
                         hls::stream<IntVectorStream_dt<8, 1> >& outStream) {
    // write data and indicate if it should be raw block or not
    bool not_done = true;
    bool rawFlagNotSent = true;
    IntVectorStream_dt<8, 1> inVal;
stream_data:
    while (not_done) {
        // read data size in bytes
        uint32_t dataSize = 0;
        inVal.strobe = 1;
        rawFlagNotSent = true;
    send_in_block:
        while (inVal.strobe > 0 && dataSize < BLOCK_SIZE) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            if (inVal.strobe > 0) {
                outStream << inVal;
                ++dataSize;
                // indicate if more data than minimum block size
                if (dataSize > MIN_BLK_SIZE && rawFlagNotSent) {
                    rawBlockFlagStream << false;
                    rawFlagNotSent = false;
                }
            }
        }
        if (dataSize > 0 && dataSize < 1 + MIN_BLK_SIZE) rawBlockFlagStream << true;
        // end of block for last block with data
        if (dataSize > 0 && inVal.strobe == 0) outStream << inVal;
        // end of block/file
        inVal.strobe = 0;
        outStream << inVal;
        // terminate condition
        not_done = (dataSize == BLOCK_SIZE);
    }
    // for end of files, value must be false
    rawBlockFlagStream << false;
}

// Content of called function
void __inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 1> inVal;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            if (!isRawBlock) outStream << inVal;
            outStrdStream << inVal;
            ++dataSize;
        }
        // End of block/file
        if (!isRawBlock) outStream << inVal;
        outStrdStream << inVal;
        // write meta data
        if (dataSize > 0) {
            // send metadata to packer
            metaVal.strobe = 1;
            metaVal.data[0] = dataSize;
            blockMetaStream << metaVal;
            if (BLOCK_SIZE > (64 * 1024)) {
                metaVal.data[0] = dataSize >> 16;
            } else {
                metaVal.data[0] = 0;
            }
            blockMetaStream << metaVal;
        } else {
            // strobe 0 for end of data, exit condition
            metaVal.strobe = 0;
            blockMetaStream << metaVal;
            break;
        }
    }
}

// Content of called function
void bufferDownsizer(hls::stream<ap_uint<IN_DATAWIDTH + SIZE_DWIDTH> >& inStream,
                     hls::stream<IntVectorStream_dt<OUT_DATAWIDTH, 1> >& outStream) {
    constexpr int16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr int16_t c_outWord = OUT_DATAWIDTH / 8;
    IntVectorStream_dt<OUT_DATAWIDTH, 1> outVal;

downsizer_top:
    while (1) {
        ap_uint<SIZE_DWIDTH> dsize = 0;
        auto inVal = inStream.read();
        // proceed further if valid size
        ap_uint<SIZE_DWIDTH> inSize = inVal.range(SIZE_DWIDTH - 1, 0);
        if (inSize == 0) break;
        auto outSizeV = ((inSize - 1) / c_outWord) + 1;
        outVal.strobe = 1;
    downsizer_assign:
        while (inSize > 0) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = inVal.range(OUT_DATAWIDTH + SIZE_DWIDTH - 1, SIZE_DWIDTH);
            inVal >>= OUT_DATAWIDTH;
            outStream << outVal;
            dsize += c_outWord;
            if (dsize == outSizeV) {
                inVal = inStream.read();
                inSize = inVal.range(SIZE_DWIDTH - 1, 0);
                dsize = 0;
                outSizeV = ((inSize - 1) / c_outWord) + 1;
            }
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}

// Content of called function
void zstdTreegenStream(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                       hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                       hls::stream<IntVectorStream_dt<4, 1> >& outWeightStream,
                       hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
#pragma HLS DATAFLOW
    hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> > heapStream("heapStream");
    hls::stream<ap_uint<9> > heapLenStream("heapLenStream");
    hls::stream<DSVectorStream_dt<Codeword, 1> > hufCodeStream("hufCodeStream");
    hls::stream<bool> eobStream("eobStream");
    hls::stream<bool> eoBlocks[2];

    details::zstdFreqFilterSort<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH>(inFreqStream, heapStream,
                                                                                          heapLenStream, eobStream);
    details::streamDistributor<2>(eobStream, eoBlocks);
    details::zstdGetHuffmanCodes<details::c_maxLitV + 1, c_tgnSymbolBits, MAX_FREQ_DWIDTH, MAX_BITS>(
        heapStream, heapLenStream, eoBlocks[0], hufCodeStream);
    details::huffCodeWeightDistributor<details::c_maxLitV + 1, MAX_FREQ_DWIDTH, MAX_BITS, 4>(
        hufCodeStream, eoBlocks[1], outCodeStream, outWeightStream, weightFreqStream);
}

// Content of called function
void zstdFreqFilterSort(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreqStream,
                        hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                        hls::stream<ap_uint<9> >& heapLenStream,
                        hls::stream<bool>& eobStream) {
    // internal buffers
    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
    bool last = false;

    hls::stream<ap_uint<SYMBOL_BITS> > maxCodes("maxCodes");
#pragma HLS STREAM variable = maxCodes depth = 2

filter_sort_ldblock:
    while (!last) {
        // filter-sort for literals
        ap_uint<9> i_symbolSize = SYMBOL_SIZE; // current symbol size
        uint16_t heapLength = 0;

        // filter the input, write 0 heapLength at end of block
        filter<MAX_FREQ_DWIDTH, 0>(inFreqStream, heap, &heapLength, maxCodes, i_symbolSize);
        // dump maxcode
        // maxCodes.read();
        // check for end of block
        last = (heapLength == 0xFFFF);
        eobStream << last;
        if (last) break;

        // sort the input
        radixSort<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength);

        // send sorted frequencies
        heapLenStream << heapLength;
        hpVal.strobe = 1;
        for (uint16_t i = 0; i < heapLength; i++) {
            hpVal.data[0] = heap[i];
            heapStream << hpVal;
        }
    }
}

// Content of called function
void radixSort(Symbol<MAX_FREQ_DWIDTH>* heap, uint16_t n) {
    //#pragma HLS INLINE
    Symbol<MAX_FREQ_DWIDTH> prev_sorting[SYMBOL_SIZE];
    Digit current_digit[SYMBOL_SIZE];
    bool not_sorted = true;

    ap_uint<SYMBOL_BITS> digit_histogram[RADIX], digit_location[RADIX];
#pragma HLS ARRAY_PARTITION variable = digit_location complete dim = 1
#pragma HLS ARRAY_PARTITION variable = digit_histogram complete dim = 1

radix_sort:
    for (uint8_t shift = 0; shift < MAX_FREQ_DWIDTH && not_sorted; shift += BITS_PER_LOOP) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 5 avg = 4
    init_histogram:
        for (uint8_t i = 0; i < RADIX; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_histogram[i] = 0;
        }

        auto prev_freq = heap[0].frequency;
        not_sorted = false;
    compute_histogram:
        for (uint16_t j = 0; j < n; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
            Symbol<MAX_FREQ_DWIDTH> val = heap[j];
            Digit digit = (val.frequency >> shift) & (RADIX - 1);
            current_digit[j] = digit;
            ++digit_histogram[digit];
            prev_sorting[j] = val;
            // check if not already in sorted order
            if (prev_freq > val.frequency) not_sorted = true;
            prev_freq = val.frequency;
        }
        digit_location[0] = 0;

    find_digit_location:
        for (uint8_t i = 0; (i < RADIX - 1) && not_sorted; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 16 avg = 16
#pragma HLS PIPELINE II = 1
            digit_location[i + 1] = digit_location[i] + digit_histogram[i];
        }

    re_sort:
        for (uint16_t j = 0; j < n && not_sorted; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE II = 1
            Digit digit = current_digit[j];
            heap[digit_location[digit]] = prev_sorting[j];
            ++digit_location[digit];
        }
    }
}

// Content of called function
void filter(Frequency<MAX_FREQ_DWIDTH>* inFreq,
            Symbol<MAX_FREQ_DWIDTH>* heap,
            uint16_t* heapLength,
            uint16_t* symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    uint16_t smLen = 0;
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        auto cf = inFreq[n];
        if (n == 256) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = 1;
            ++hpLen;
        } else if (cf != 0) {
            heap[hpLen].value = smLen = n;
            heap[hpLen].frequency = cf;
            ++hpLen;
        }
    }

    heapLength[0] = hpLen;
    symLength[0] = smLen;
}

// Content of called function
void huffCodeWeightDistributor(hls::stream<DSVectorStream_dt<Codeword, 1> >& hufCodeStream,
                               hls::stream<bool>& isEOBlocks,
                               hls::stream<DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> >& outCodeStream,
                               hls::stream<IntVectorStream_dt<BLEN_BITS, 1> >& outWeightsStream,
                               hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& weightFreqStream) {
    // distribute huffman codes to multiple output streams and one separate bitlen stream
    DSVectorStream_dt<HuffmanCode_dt<MAX_BITS>, 1> outCode;
    IntVectorStream_dt<BLEN_BITS, 1> outWeight;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outFreq;
    int blk_n = 0;
distribute_code_block:
    while (isEOBlocks.read() == false) {
        ++blk_n;
        ap_uint<MAX_FREQ_DWIDTH> weightFreq[MAX_BITS + 1];
        ap_uint<BLEN_BITS> blenBuf[SYMBOL_SIZE];
#pragma HLS ARRAY_PARTITION variable = weightFreq complete
#pragma HLS BIND_STORAGE variable = blenBuf type = ram_1p impl = lutram
        ap_uint<BLEN_BITS> curMaxBitlen = 0;
        uint8_t maxWeight = 0;
        uint16_t maxVal = 0;
    init_freq_bl:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE off
            weightFreq[i] = 0;
        }
        outCode.strobe = 1;
        outWeight.strobe = 1;
        outFreq.strobe = 1;
    distribute_code_loop:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto inCode = hufCodeStream.read();
            uint8_t hfblen = inCode.data[0].bitlength;
            uint16_t hfcode = inCode.data[0].codeword;
            outCode.data[0].code = hfcode;
            outCode.data[0].bitlen = hfblen;
            blenBuf[i] = hfblen;
            if (hfblen > curMaxBitlen) curMaxBitlen = hfblen;
            if (hfblen > 0) {
                maxVal = (uint16_t)i;
            }
            outCodeStream << outCode;
        }
    send_weights:
        for (ap_uint<9> i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS PIPELINE II = 1
            auto bitlen = blenBuf[i];
            auto blenWeight = (uint8_t)((bitlen > 0) ? (uint8_t)(curMaxBitlen + 1 - bitlen) : 0);
            outWeight.data[0] = blenWeight;
            weightFreq[blenWeight] += (uint8_t)(i < maxVal + 1); // conditional increment
            outWeightsStream << outWeight;
        }
        // write maxVal as first value
        outFreq.data[0] = maxVal;
        weightFreqStream << outFreq;
    // send weight frequencies
    send_weight_freq:
        for (uint8_t i = 0; i < MAX_BITS + 1; ++i) {
#pragma HLS PIPELINE II = 1
            outFreq.data[0] = weightFreq[i];
            weightFreqStream << outFreq;
            if (outFreq.data[0] > 0) maxWeight = i; // to be deduced by module reading this stream
        }
        // end of block
        outCode.strobe = 0;
        outWeight.strobe = 0;
        outFreq.strobe = 0;
        outCodeStream << outCode;
        outWeightsStream << outWeight;
        weightFreqStream << outFreq;
    }
    // end of all data
    outCode.strobe = 0;
    outWeight.strobe = 0;
    outFreq.strobe = 0;
    outCodeStream << outCode;
    outWeightsStream << outWeight;
    weightFreqStream << outFreq;
}

// Content of called function
void zstdGetHuffmanCodes(hls::stream<DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> >& heapStream,
                         hls::stream<ap_uint<9> >& heapLenStream,
                         hls::stream<bool>& isEOBlocks,
                         hls::stream<DSVectorStream_dt<Codeword, 1> >& outCodes) {
    ap_uint<SYMBOL_SIZE> left = 0;
    ap_uint<SYMBOL_SIZE> right = 0;
    ap_uint<SYMBOL_BITS> parent[SYMBOL_SIZE];
    Histogram length_histogram[c_lengthHistogram];
#pragma HLS ARRAY_PARTITION variable = length_histogram complete
    Frequency<MAX_FREQ_DWIDTH> temp_array[SYMBOL_SIZE];

    Symbol<MAX_FREQ_DWIDTH> heap[SYMBOL_SIZE];
#pragma HLS BIND_STORAGE variable = heap type = ram_t2p impl = bram
#pragma HLS AGGREGATE variable = heap

    ap_uint<4> symbol_bits[SYMBOL_SIZE];
    DSVectorStream_dt<Symbol<MAX_FREQ_DWIDTH>, 1> hpVal;
construct_tree_ldblock:
    while (isEOBlocks.read() == false) {
        uint16_t heapLength = heapLenStream.read();
    init_buffers:
        for (uint16_t i = 0; i < SYMBOL_SIZE; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
#pragma HLS PIPELINE II = 1
            parent[i] = 0;
            if (i < c_lengthHistogram) length_histogram[i] = 0;
            temp_array[i] = 0;
            if (i < heapLength) {
                hpVal = heapStream.read();
                heap[i] = hpVal.data[0];
            }
            symbol_bits[i] = 0;
        }

        // create tree
        createTree<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, parent, left, right, temp_array);

        // get bit-lengths from tree
        computeBitLength<SYMBOL_SIZE, SYMBOL_BITS, MAX_FREQ_DWIDTH>(parent, left, right, heapLength, length_histogram,
                                                                    temp_array);

        // truncate tree for any bigger bit-lengths
        truncateTree(length_histogram, c_lengthHistogram, MAX_BITS);

        // canonize the tree
        canonizeTree<SYMBOL_BITS, MAX_FREQ_DWIDTH>(heap, heapLength, length_histogram, symbol_bits, MAX_BITS + 1);

        // generate huffman codewords
        createZstdCodeword<MAX_BITS>(symbol_bits, length_histogram, outCodes, SYMBOL_SIZE, MAX_BITS, heapLength);
    }
}

// Content of called function
void canonizeTree(Symbol<MAX_FREQ_DWIDTH>* sorted,
                  uint32_t num_symbols,
                  Histogram* length_histogram,
                  ap_uint<4>* symbol_bits,
                  uint16_t i_treeDepth) {
    int16_t length = i_treeDepth;
    ap_uint<SYMBOL_BITS> count = 0;
// Iterate across the symbols from lowest frequency to highest
// Assign them largest bit length to smallest
process_symbols:
    for (uint32_t k = 0; k < num_symbols; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 256 avg = 286
        if (count == 0) {
            // find the next non-zero bit length k
            count = length_histogram[--length];
        canonize_inner:
            while (count == 0 && length >= 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 avg = 1 max = 2
#pragma HLS PIPELINE II = 1
                // n  is the number of symbols with encoded length k
                count = length_histogram[--length];
            }
        }
        if (length < 0) break;
        symbol_bits[sorted[k].value] = length; // assign symbol k to have length bits
        --count;                               // keep assigning i bits until we have counted off n symbols
    }
}

// Content of called function
void truncateTree(Histogram* length_histogram, uint16_t c_tree_depth, int max_bit_len) {
    int j = max_bit_len;
move_nodes:
    for (uint16_t i = c_tree_depth - 1; i > max_bit_len; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 572 max = 572 avg = 572
#pragma HLS PIPELINE II = 1
        // Look to see if there are any nodes at lengths greater than target depth
        int cnt = 0;
    reorder:
        for (; length_histogram[i] != 0;) {
#pragma HLS LOOP_TRIPCOUNT min = 3 max = 3 avg = 3
            if (j == max_bit_len) {
                // find the deepest leaf with codeword length < target depth
                --j;
            trctr_mv:
                while (length_histogram[j] == 0) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 1 avg = 1
                    --j;
                }
            }
            // Move leaf with depth i to depth j + 1
            length_histogram[j] -= 1;     // The node at level j is no longer a leaf
            length_histogram[j + 1] += 2; // Two new leaf nodes are attached at level j+1
            length_histogram[i - 1] += 1; // The leaf node at level i+1 gets attached here
            length_histogram[i] -= 2;     // Two leaf nodes have been lost from  level i

            // now deepest leaf with codeword length < target length
            // is at level (j+1) unless (j+1) == target length
            ++j;
        }
    }
}

// Content of called function
void createZstdCodeword(ap_uint<4>* symbol_bits,
                        Histogram* length_histogram,
                        hls::stream<DSVectorStream_dt<Codeword, 1> >& huffCodes,
                        uint16_t cur_symSize,
                        uint16_t cur_maxBits,
                        uint16_t symCnt) {
    //#pragma HLS inline
    bool allSameBlen = true;
    typedef ap_uint<MAX_LEN> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_LEN + 1];
    //#pragma HLS ARRAY_PARTITION variable = first_codeword complete dim = 1

    DSVectorStream_dt<Codeword, 1> hfc;
    hfc.strobe = 1;

    // Computes the initial codeword value for a symbol with bit length i
    first_codeword[0] = 0;
    uint8_t uniq_bl_idx = 0;
find_uniq_blen_count:
    for (uint8_t i = 0; i < cur_maxBits + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 12
        if (length_histogram[i] == cur_symSize) uniq_bl_idx = i;
    }
    // If only 1 uniq_blc for all symbols divide into 3 bitlens
    // Means, if all the bitlens are same(mainly bitlen-8) then use an alternate tree
    // Fix the current bitlength_histogram and symbol_bits so that it generates codes-bitlens for alternate tree
    if (uniq_bl_idx > 0) {
        length_histogram[7] = 1;
        length_histogram[9] = 2;
        length_histogram[8] -= 3;

        symbol_bits[0] = 7;
        symbol_bits[1] = 9;
        symbol_bits[2] = 9;
    }

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = cur_maxBits; i > 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 11
        uint16_t cur = nextCode;
        nextCode += (length_histogram[i]);
        nextCode >>= 1;
        first_codeword[i] = cur;
    }
    Codeword code;
assign_codewords_sm:
    for (uint16_t k = 0; k < cur_symSize; ++k) {
#pragma HLS LOOP_TRIPCOUNT max = 256 min = 256 avg = 256
#pragma HLS PIPELINE II = 1
        uint8_t length = (uint8_t)symbol_bits[k];
        // length = (uniq_bl_idx > 0 && k > 2 && length > 8) ? 8 : length;	// not needed if treegen is optimal
        length = (symCnt == 0) ? 0 : length;
        code.codeword = (uint16_t)first_codeword[length];
        // get bitlength for code
        length = (symCnt == 0 && k == 0) ? 1 : length;
        code.bitlength = length;
        // write out codes
        hfc.data[0] = code;
        first_codeword[length]++;
        huffCodes << hfc;
    }
}

// Content of called function
void createTree(Symbol<MAX_FREQ_DWIDTH>* heap,
                int num_symbols,
                ap_uint<SYMBOL_BITS>* parent,
                ap_uint<SYMBOL_SIZE>& left,
                ap_uint<SYMBOL_SIZE>& right,
                Frequency<MAX_FREQ_DWIDTH>* frequency) {
    ap_uint<SYMBOL_BITS> tree_count = 0; // Number of intermediate node assigned a parent
    ap_uint<SYMBOL_BITS> in_count = 0;   // Number of inputs consumed
    ap_uint<SYMBOL_SIZE> tmp;
    left = 0;
    right = 0;

    // for case with less number of symbols
    if (num_symbols < 3) num_symbols++;
// this loop needs to run at least twice
create_heap:
    for (uint16_t i = 0; i < num_symbols; ++i) {
#pragma HLS PIPELINE II = 3
#pragma HLS LOOP_TRIPCOUNT min = 19 avg = 286 max = 286
        Frequency<MAX_FREQ_DWIDTH> node_freq = 0;
        Frequency<MAX_FREQ_DWIDTH> intermediate_freq = frequency[tree_count];
        Frequency<MAX_FREQ_DWIDTH> symFreq = heap[in_count].frequency;
        tmp = 1;
        tmp <<= i;

        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // left[i] = s.value;       // set input symbol value as left node
            node_freq = symFreq; // Add symbol frequency to total node frequency
            // move to the next input symbol
            ++in_count;
        } else {
            // pick internal node without a parent
            // left[i] = INTERNAL_NODE;           // Set symbol to indicate an internal node
            left |= tmp;
            node_freq = intermediate_freq; // Add child node frequency
            parent[tree_count] = i;        // Set this node as child's parent
            // Go to next parent-less internal node
            ++tree_count;
        }

        intermediate_freq = frequency[tree_count];
        symFreq = heap[in_count].frequency;
        if ((in_count < num_symbols && symFreq <= intermediate_freq) || tree_count == i) {
            // Pick symbol from heap
            // right[i] = s.value;
            frequency[i] = node_freq + symFreq;
            ++in_count;
        } else {
            // Pick internal node without a parent
            // right[i] = INTERNAL_NODE;
            right |= tmp;
            frequency[i] = node_freq + intermediate_freq;
            parent[tree_count] = i;
            ++tree_count;
        }
    }
}

// Content of called function
void computeBitLength(ap_uint<SYMBOL_BITS>* parent,
                      ap_uint<SYMBOL_SIZE>& left,
                      ap_uint<SYMBOL_SIZE>& right,
                      int num_symbols,
                      Histogram* length_histogram,
                      Frequency<MAX_FREQ_DWIDTH>* child_depth) {
    ap_uint<SYMBOL_SIZE> tmp;
    // for case with less number of symbols
    if (num_symbols < 2) num_symbols++;
    // Depth of the root node is 0.
    child_depth[num_symbols - 1] = 0;
// this loop needs to run at least once
// II is 1 or 2 depending on configuration of memory
// used for arrays "child_depth" and "length_histogram"
traverse_tree:
    for (int16_t i = num_symbols - 2; i >= 0; --i) {
#pragma HLS LOOP_TRIPCOUNT min = 19 max = 286
#pragma HLS PIPELINE
        tmp = 1;
        tmp <<= i;
        uint32_t length = child_depth[parent[i]] + 1;
        child_depth[i] = length;
        bool is_left_internal = ((left & tmp) == 0);
        bool is_right_internal = ((right & tmp) == 0);

        if ((is_left_internal || is_right_internal)) {
            uint32_t children = 1; // One child of the original node was a symbol
            if (is_left_internal && is_right_internal) {
                children = 2; // Both the children of the original node were symbols
            }
            length_histogram[length] += children;
        }
    }
}

// Content of called function
void streamDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 1) break;
    } while (1);
}

// Content of called function
void simpleStreamUpsizer(hls::stream<IntVectorStream_dt<8, IN_WIDTH / 8> >& inStream,
                         hls::stream<ap_uint<OUT_WIDTH + SIZE_DWIDTH> >& outStream) {
    constexpr uint8_t c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    constexpr uint8_t c_inBytes = IN_WIDTH / 8;
    ap_uint<IN_WIDTH> inVal;
    ap_uint<OUT_WIDTH> outVal;
    bool last = false;
    ap_uint<4> dsize;

stream_upsizer_top:
    while (!last) {
        last = true;
        uint8_t byteIdx = 0;
        dsize = 0;
        auto inStVal = inStream.read();
        bool loop_continue = (inStVal.strobe != 0);
    stream_upsizer_main:
        while (loop_continue) {
#pragma HLS PIPELINE II = 1
            last = false;
            if (byteIdx == c_upsizeFactor) {
                ap_uint<SIZE_DWIDTH + OUT_WIDTH> tmpVal = outVal;
                tmpVal <<= SIZE_DWIDTH;
                tmpVal.range(SIZE_DWIDTH - 1, 0) = dsize;
                outStream << tmpVal;
                byteIdx = 0;
                dsize = 0;
                loop_continue = (inStVal.strobe != 0);
            }
        upszr_assign_input:
            for (uint8_t b = 0; b < c_inBytes; ++b) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 0 max = c_inBytes
                if (b < inStVal.strobe) inVal.range(((b + 1) * 8) - 1, b * 8) = inStVal.data[b];
            }
            outVal >>= IN_WIDTH;
            outVal.range(OUT_WIDTH - 1, OUT_WIDTH - IN_WIDTH) = inVal;
            ++byteIdx;
            dsize += inStVal.strobe;
            if (inStVal.strobe != 0) inStVal = inStream.read();
        }
        // end of block/files
        outStream << 0;
    }
}

// Content of called function
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

// Content of called function
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

// Content of called function
void bufferDownsizerVec(hls::stream<ap_uint<IN_DATAWIDTH + SIZE_DWIDTH> >& inStream,
                        hls::stream<IntVectorStream_dt<8, OUT_DATAWIDTH / 8> >& outStream) {
    constexpr uint16_t c_factor = IN_DATAWIDTH / OUT_DATAWIDTH;
    constexpr uint8_t c_outWord = OUT_DATAWIDTH / 8;
    constexpr uint8_t c_outDataHigh = OUT_DATAWIDTH + SIZE_DWIDTH - 1;
    IntVectorStream_dt<8, c_outWord> outVal;

downsizer_top:
    while (1) {
        auto inVal = inStream.read();
        // proceed further if valid size
        ap_uint<SIZE_DWIDTH> inSize = inVal.range(SIZE_DWIDTH - 1, 0);
        if (inSize == 0) break;
    downsizer_assign:
        while (inSize > 0) {
#pragma HLS PIPELINE II = 1
            ap_uint<OUT_DATAWIDTH> outReg = inVal.range(c_outDataHigh, SIZE_DWIDTH);
            inVal >>= OUT_DATAWIDTH;
            outVal.strobe = ((inSize < c_outWord) ? (uint8_t)inSize : c_outWord);
            for (uint8_t i = 0; i < c_outWord; ++i) {
#pragma HLS UNROLL
                outVal.data[i] = outReg.range((i * 8) + 7, i * 8);
            }
            outStream << outVal;
            inSize -= outVal.strobe;
            if (inSize == 0) {
                inVal = inStream.read();
                inSize = inVal.range(SIZE_DWIDTH - 1, 0);
            }
        }
        // Block end Condition
        outVal.strobe = 0;
        outStream << outVal;
    }
    // File end Condition
    outVal.strobe = 0;
    outStream << outVal;
}

// Content of called function
void zstdAxiu2hlsStream(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                        hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> >& outHlsStream) {
    constexpr uint8_t c_keepDWidth = IN_DWIDTH / 8;
    IntVectorStream_dt<8, c_keepDWidth> outVal;
    outVal.strobe = c_keepDWidth;
    auto inAxiVal = inStream.read();
axi_to_hls:
    for (; inAxiVal.last == false; inAxiVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outHlsStream << outVal;
    }
    uint8_t strb = countSetBits<c_keepDWidth>((int)(inAxiVal.keep));
    if (strb) { // write last byte if valid
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outVal.strobe = strb;
        outHlsStream << outVal;
    }
    outVal.strobe = 0;
    outHlsStream << outVal;
}

// Content of called function
uint8_t countSetBits(ap_uint<DWIDTH> val) {
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < DWIDTH; ++i) {
#pragma HLS UNROLL
        cnt += val.range(i, i);
    }
    return cnt;
}

// Content of called function
void zstdHlsVectorStream2axiu(hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> >& hlsInStream,
                              hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    constexpr uint8_t c_bytesInWord = OUT_DWIDTH / 8;
    ap_axiu<OUT_DWIDTH, 0, 0, 0> outVal;
    ap_uint<OUT_DWIDTH * 2> outReg;
    uint8_t bytesInReg = 0;
    outVal.keep = -1;
    outVal.last = false;
    uint32_t outCnt = 0;
hls_to_axi:
    for (auto inVal = hlsInStream.read(); inVal.strobe > 0; inVal = hlsInStream.read()) {
#pragma HLS PIPELINE II = 1
        // Write data to output register
        for (uint8_t i = 0; i < c_bytesInWord; ++i) {
#pragma HLS UNROLL
            uint16_t idx = (bytesInReg + i) * 8;
            outReg.range(idx + 7, idx) = inVal.data[i];
        }
        bytesInReg += inVal.strobe;
        outCnt += inVal.strobe;
        if (bytesInReg > c_bytesInWord - 1) {
            outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
            outStream << outVal;
            outReg >>= OUT_DWIDTH;
            bytesInReg -= c_bytesInWord;
        }
    }
    if (bytesInReg) {
        outVal.keep = (((uint16_t)1 << bytesInReg) - 1);
        outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
        outStream << outVal;
    }
    // send eos
    outVal.last = true;
    outVal.keep = 0;
    outVal.data = 0;
    outStream << outVal;
}