void zstdCompressMultiCoreStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                                    hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    // Internal streams
    hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> > inZstdStream("inZstdStream");
    hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> > outCompressedStream("outCompressedStream");
#pragma HLS STREAM variable = inZstdStream depth = 8
#pragma HLS STREAM variable = outCompressedStream depth = 8

#pragma HLS DATAFLOW
    // AXI 2 HLS Stream
    xf::compression::details::zstdAxiu2hlsStream<IN_DWIDTH>(inStream, inZstdStream);

    // Zstd Compress Stream IO Engine
    xf::compression::zstdCompressQuadCore<CORE_COUNT, BLOCK_SIZE, LZWINDOW_SIZE, MIN_BLCK_SIZE>(inZstdStream,
                                                                                                outCompressedStream);
    // HLS 2 AXI Stream
    xf::compression::details::zstdHlsVectorStream2axiu<OUT_DWIDTH>(outCompressedStream, outStream);
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
void zstdCompressQuadCore(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    // zstd compression main module
    constexpr uint8_t c_parallel_huffman = (CORE_COUNT < 4) ? 2 : (CORE_COUNT < 6 ? 4 : 8);
    constexpr uint32_t c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr uint32_t c_dataUpSDepth = BLOCK_SIZE / 8;
    constexpr uint32_t c_seqInSDepth = BLOCK_SIZE / 4;
    constexpr uint32_t c_rawBlkSDepth = CORE_COUNT * BLOCK_SIZE / 8;
    constexpr uint32_t c_hfLitStreamDepth = BLOCK_SIZE / c_parallel_huffman;
    constexpr uint32_t c_seqBlockDepth = BLOCK_SIZE / 8;
    constexpr uint8_t c_wfreqSDepth = 24 * CORE_COUNT;

    // Internal streams
    hls::stream<ap_uint<68> > inBlockStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<8, 1> > inBlockDszStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
    hls::stream<ap_uint<68> > rawBlockStream("rawBlockStream");
    hls::stream<ap_uint<68> > rawBlkFinalStream("rawBlkFinalStream");
#pragma HLS STREAM variable = inBlockStream depth = c_dataUpSDepth
#pragma HLS STREAM variable = inBlockDszStream depth = 16
#pragma HLS STREAM variable = packerMetaStream depth = 16
#pragma HLS STREAM variable = rawBlkFinalStream depth = 16
#pragma HLS STREAM variable = rawBlockStream depth = c_rawBlkSDepth
#pragma HLS BIND_STORAGE variable = rawBlockStream type = FIFO impl = URAM

    hls::stream<IntVectorStream_dt<8, 1> > litStream[CORE_COUNT];
    hls::stream<ap_uint<68> > litUpsizedStream[CORE_COUNT];
    hls::stream<ap_uint<68> > serialLitUpszStream("serialLitUpszStream");
    hls::stream<ap_uint<68> > reverseLitStream("reverseLitStream");
#pragma HLS STREAM variable = litStream depth = 64
#pragma HLS STREAM variable = reverseLitStream depth = 64
#pragma HLS STREAM variable = serialLitUpszStream depth = 8
#pragma HLS STREAM variable = litUpsizedStream depth = c_dataUpSDepth

    hls::stream<DSVectorStream_dt<details::SequencePack<c_freq_dwidth, 8>, 1> > seqStream[CORE_COUNT];
    hls::stream<DSVectorStream_dt<details::SequencePack<c_freq_dwidth, 8>, 1> > serialSeqStream("serialSeqStream");
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream("reverseSeqStream");
#pragma HLS STREAM variable = seqStream depth = c_seqInSDepth
#pragma HLS AGGREGATE variable = seqStream
#pragma HLS STREAM variable = serialSeqStream depth = 8
#pragma HLS STREAM variable = reverseSeqStream depth = 2048 // 4096
// 2-4K depth needed to keep reading input sequences even if previous block decoding waits for fse table generation
#pragma HLS BIND_STORAGE variable = reverseSeqStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = seqStream type = FIFO impl = BRAM

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialLitFreqStream("serialLitFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialSeqFreqStream("serialSeqFreqStream");
#pragma HLS STREAM variable = litFreqStream depth = 16
#pragma HLS STREAM variable = seqFreqStream depth = 128
#pragma HLS STREAM variable = serialLitFreqStream depth = 8
#pragma HLS STREAM variable = serialSeqFreqStream depth = 8

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
#pragma HLS STREAM variable = wghtFreqStream depth = c_wfreqSDepth
#pragma HLS STREAM variable = freqStream depth = 128

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lzMetaStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialLzMetaStream("serialLzMetaStream");
    hls::stream<ap_uint<c_freq_dwidth> > bscLzMetaStream("bscLzMetaStream");
    hls::stream<bool> rleFlagStream[CORE_COUNT];
    hls::stream<bool> serialRleFlagStream("serialRleFlagStream");
    hls::stream<ap_uint<2> > rawBlockFlagStream("rawBlockFlagStream");
    hls::stream<ap_uint<2> > rwbFinalFlagStream1("rwbFinalFlagStream1");
    hls::stream<ap_uint<2> > rwbFinalFlagStream2("rwbFinalFlagStream2");
    hls::stream<ap_uint<c_freq_dwidth> > litCntStream[CORE_COUNT];
    hls::stream<ap_uint<c_freq_dwidth> > serialLitCntStream("serialLitCntStream");
#pragma HLS STREAM variable = lzMetaStream depth = 16
#pragma HLS STREAM variable = serialLzMetaStream depth = 16
#pragma HLS STREAM variable = bscLzMetaStream depth = 16
#pragma HLS STREAM variable = rleFlagStream depth = 4
#pragma HLS STREAM variable = serialRleFlagStream depth = 4
#pragma HLS STREAM variable = rawBlockFlagStream depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream1 depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream2 depth = 4
#pragma HLS STREAM variable = litCntStream depth = 8
#pragma HLS STREAM variable = serialLitCntStream depth = 8

    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");
#pragma HLS STREAM variable = fseHeaderStream depth = 128
#pragma HLS STREAM variable = fseLitTableStream depth = 8
#pragma HLS STREAM variable = fseSeqTableStream depth = 8

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, c_parallel_huffman> > hfEncodedLitStream;
    hls::stream<ap_uint<c_parallel_huffman * details::c_maxZstdHfBits> > hfLitBitstream("hfLitBitstream");
    hls::stream<ap_uint<8> > hfMultiBlenStream("hfMultiBlenStream");
    hls::stream<IntVectorStream_dt<8, c_parallel_huffman> > hfEncBitStream("hfEncBitStream");
#pragma HLS STREAM variable = hufLitMetaStream depth = 256
#pragma HLS STREAM variable = hufCodeStream depth = 16
#pragma HLS STREAM variable = hufWeightStream depth = 16
#pragma HLS STREAM variable = hfEncodedLitStream depth = 16
#pragma HLS STREAM variable = hfLitBitstream depth = 16
#pragma HLS STREAM variable = hfMultiBlenStream depth = 16
#pragma HLS STREAM variable = hfEncBitStream depth = c_hfLitStreamDepth
#pragma HLS BIND_STORAGE variable = hfEncBitStream type = FIFO impl = BRAM

    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<ap_uint<c_freq_dwidth> > seqEncSizeStream("seqEncSizeStream");
    hls::stream<IntVectorStream_dt<8, 8> > seqEncodedStream("seqEncodedStream");
#pragma HLS STREAM variable = litEncodedStream depth = 128
#pragma HLS STREAM variable = seqEncSizeStream depth = 4
#pragma HLS STREAM variable = seqEncodedStream depth = c_seqBlockDepth
    hls::stream<ap_uint<c_freq_dwidth> > bscMetaStream("bscMetaStream");
    hls::stream<IntVectorStream_dt<8, 8> > bscBitstream("bscBitstream");
    hls::stream<IntVectorStream_dt<8, 8> > cmpFrameStream("cmpFrameStream");
    hls::stream<IntVectorStream_dt<8, 8> > cmpFrameFinalStream("cmpFrameFinalStream");
#pragma HLS STREAM variable = bscMetaStream depth = 128
#pragma HLS STREAM variable = bscBitstream depth = 128
#pragma HLS STREAM variable = cmpFrameStream depth = 64
#pragma HLS STREAM variable = cmpFrameFinalStream depth = 16

    // select URAM vs BRAMs for streams
    if (BLOCK_SIZE < 32768) {
#pragma HLS BIND_STORAGE variable = inBlockStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = BRAM
    } else {
#pragma HLS BIND_STORAGE variable = inBlockStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = URAM
    }

#pragma HLS dataflow

    // Module-1: Input reading and LZ77 compression
    {
        details::inputDistributer<BLOCK_SIZE, MIN_BLCK_SIZE, CORE_COUNT>(inStream, inBlockStream, rawBlockStream,
                                                                         packerMetaStream);
        for (uint8_t coreIdx = 0; coreIdx < CORE_COUNT; ++coreIdx) {
#pragma HLS UNROLL
            details::bufferDownsizerVec<64, 8, 4>(inBlockStream[coreIdx], inBlockDszStream[coreIdx]);
        }
        // LZ77 compression of input blocks to get separate streams
        // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
        details::getLitSequences<0, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[0], litStream[0], seqStream[0],
                                                               litFreqStream[0], seqFreqStream[0], rleFlagStream[0],
                                                               lzMetaStream[0], litCntStream[0]);
        if (CORE_COUNT > 1) {
            details::getLitSequences<1, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[1], litStream[1], seqStream[1],
                                                                   litFreqStream[1], seqFreqStream[1], rleFlagStream[1],
                                                                   lzMetaStream[1], litCntStream[1]);
        }
        // Quad Core
        if (CORE_COUNT > 2) {
            details::getLitSequences<2, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[2], litStream[2], seqStream[2],
                                                                   litFreqStream[2], seqFreqStream[2], rleFlagStream[2],
                                                                   lzMetaStream[2], litCntStream[2]);
            details::getLitSequences<3, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[3], litStream[3], seqStream[3],
                                                                   litFreqStream[3], seqFreqStream[3], rleFlagStream[3],
                                                                   lzMetaStream[3], litCntStream[3]);
        }
        // Hexa Core
        if (CORE_COUNT > 4) {
            details::getLitSequences<4, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[4], litStream[4], seqStream[4],
                                                                   litFreqStream[4], seqFreqStream[4], rleFlagStream[4],
                                                                   lzMetaStream[4], litCntStream[4]);
            details::getLitSequences<5, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[5], litStream[5], seqStream[5],
                                                                   litFreqStream[5], seqFreqStream[5], rleFlagStream[5],
                                                                   lzMetaStream[5], litCntStream[5]);
        }

        // Octa Core
        if (CORE_COUNT > 6) {
            details::getLitSequences<6, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[6], litStream[6], seqStream[6],
                                                                   litFreqStream[6], seqFreqStream[6], rleFlagStream[6],
                                                                   lzMetaStream[6], litCntStream[6]);
            details::getLitSequences<7, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[7], litStream[7], seqStream[7],
                                                                   litFreqStream[7], seqFreqStream[7], rleFlagStream[7],
                                                                   lzMetaStream[7], litCntStream[7]);
        }

        for (uint8_t coreIdx = 0; coreIdx < CORE_COUNT; ++coreIdx) {
#pragma HLS UNROLL
            // Upsize literals
            details::simpleStreamUpsizer<8, 64, 4>(litStream[coreIdx], litUpsizedStream[coreIdx]);
        }
        // serialize output literals, sequences and frequencies
        details::serializeLiterals<CORE_COUNT, c_freq_dwidth>(litUpsizedStream, litCntStream, serialLitUpszStream,
                                                              serialLitCntStream);
        details::serializeLZData<CORE_COUNT, c_freq_dwidth>(
            seqStream, litFreqStream, seqFreqStream, rleFlagStream, lzMetaStream, serialSeqStream, serialLitFreqStream,
            serialSeqFreqStream, serialRleFlagStream, serialLzMetaStream);
        // compress type detection
        details::zstdCompressionMeta<BLOCK_SIZE, c_freq_dwidth>(serialLzMetaStream, rawBlockFlagStream,
                                                                bscLzMetaStream);
        // skip raw block based on flags
        details::skipPassRawBlock<64, 4>(rawBlockStream, rawBlockFlagStream, rawBlkFinalStream, rwbFinalFlagStream1,
                                         rwbFinalFlagStream2);
    }
    // Module-2: Encoding table generation and data preparation
    {
        // Buffer, reverse and break input literal stream into 4 streams of 1/4th size
        details::reverseLitQuadStreams<BLOCK_SIZE, c_freq_dwidth>(serialLitUpszStream, serialLitCntStream,
                                                                  reverseLitStream);
        // Reverse sequences stream
        details::reverseSeq<BLOCK_SIZE, c_freq_dwidth, MIN_MATCH>(serialSeqStream, reverseSeqStream);
        // generate hufffman tree and get codes-bitlens
        zstdTreegenStream<c_freq_dwidth, details::c_maxZstdHfBits>(serialLitFreqStream, hufCodeStream, hufWeightStream,
                                                                   wghtFreqStream);
        // feed frequency data to fse table gen from literals and sequences
        details::frequencySequencer<c_freq_dwidth>(wghtFreqStream, serialSeqFreqStream, freqStream);
        // generate FSE Tables for litlen, matlen, offset and literal-bitlen
        details::fseTableGen(freqStream, fseHeaderStream, fseLitTableStream, fseSeqTableStream);
    }
    // Module-3: Encoding literal and sequences
    {
        // Huffman encoding of literal stream
        details::zstdHuffmanMultiEncoder<details::c_maxZstdHfBits, c_parallel_huffman>(
            reverseLitStream, serialRleFlagStream, hufCodeStream, hfEncodedLitStream, hufLitMetaStream);
        // Huffman bitstream packer
        details::zstdHuffMultiBitPacker<details::c_maxZstdHfBits, c_parallel_huffman>(
            hfEncodedLitStream, hfLitBitstream, hfMultiBlenStream);
        // packed bitstream alignment downsizer
        details::bitDownSizeByte<c_parallel_huffman * details::c_maxZstdHfBits, 8 * c_parallel_huffman, 8>(
            hfLitBitstream, hfMultiBlenStream, hfEncBitStream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, fseSeqTableStream, seqEncodedStream, seqEncSizeStream);
    }
    // Module-4: Output block and frame packing
    {
        // collect data from different input byte streams and output 2 continuous streams
        details::bytestreamCollector<c_freq_dwidth, 8, c_parallel_huffman>(
            bscLzMetaStream, hufLitMetaStream, hfEncBitStream, fseHeaderStream, litEncodedStream, seqEncodedStream,
            seqEncSizeStream, bscMetaStream, bscBitstream);
        // pack compressed data into single sequential block stream
        details::packCompressedFrame<BLOCK_SIZE, MIN_BLCK_SIZE, c_freq_dwidth, 8>(packerMetaStream, bscMetaStream,
                                                                                  bscBitstream, cmpFrameStream);
        details::skipPassCmpBlock<8>(cmpFrameStream, rwbFinalFlagStream1, cmpFrameFinalStream);
        // Output compressed or raw block based on input flag stream
        details::streamCmpStrdFrame(rawBlkFinalStream, cmpFrameFinalStream, rwbFinalFlagStream2, outStream);
    }
}

// Content of called function
void getLitSequences(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                     hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                     hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& seqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                     hls::stream<bool>& rleFlagStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outMetaStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
    hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> > seqFszStream("seqFszStream");
#pragma HLS STREAM variable = compressedStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = seqFszStream depth = 16

    // LZ77 compress
    xf::compression::lzCompress<BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_IDX>(
        inStream, compressedStream);
    // improve CR and generate clean sequences
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    // separate literals from sequences and generate literal frequencies
    xf::compression::details::zstdLz77DivideStream<MAX_FREQ_DWIDTH, MIN_MATCH>(
        boosterStream, litStream, seqFszStream, litFreqStream, seqFreqStream, rleFlagStream, outMetaStream,
        litCntStream);
    // Downsize literal lengths
    xf::compression::details::downSizeLitlen<MAX_FREQ_DWIDTH>(seqFszStream, seqStream);
}

// Content of called function
void lzCompress(hls::stream<IntVectorStream_dt<8, 1> >& inStream, hls::stream<IntVectorStream_dt<32, 1> >& outStream) {
    const uint16_t c_indxBitCnts = 24;
    const uint16_t c_fifo_depth = LEFT_BYTES + 2;
    const int c_dictEleWidth = (MATCH_LEN * 8 + c_indxBitCnts);
    typedef ap_uint<MATCH_LEVEL * c_dictEleWidth> uintDictV_t;
    typedef ap_uint<c_dictEleWidth> uintDict_t;
    const uint32_t totalDictSize = (1 << (c_indxBitCnts - 1)); // 8MB based on index 3 bytes
#ifndef AVOID_STATIC_MODE
    static bool resetDictFlag = true;
    static uint32_t relativeNumBlocks = 0;
#else
    bool resetDictFlag = true;
    uint32_t relativeNumBlocks = 0;
#endif

    uintDictV_t dict[LZ_DICT_SIZE];
#pragma HLS RESOURCE variable = dict core = XPM_MEMORY uram

    // local buffers for each block
    uint8_t present_window[MATCH_LEN];
#pragma HLS ARRAY_PARTITION variable = present_window complete
    hls::stream<uint8_t> lclBufStream("lclBufStream");
#pragma HLS STREAM variable = lclBufStream depth = c_fifo_depth
#pragma HLS BIND_STORAGE variable = lclBufStream type = fifo impl = srl

    // input register
    IntVectorStream_dt<8, 1> inVal;
    // output register
    IntVectorStream_dt<32, 1> outValue;
    // loop over blocks
    while (true) {
        uint32_t iIdx = 0;
        // once 8MB data is processed reset dictionary
        // 8MB based on index 3 bytes
        if (resetDictFlag) {
            ap_uint<MATCH_LEVEL* c_dictEleWidth> resetValue = 0;
            for (int i = 0; i < MATCH_LEVEL; i++) {
#pragma HLS UNROLL
                resetValue.range((i + 1) * c_dictEleWidth - 1, i * c_dictEleWidth + MATCH_LEN * 8) = -1;
            }
        // Initialization of Dictionary
        dict_flush:
            for (int i = 0; i < LZ_DICT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
                dict[i] = resetValue;
            }
            resetDictFlag = false;
            relativeNumBlocks = 0;
        } else {
            relativeNumBlocks++;
        }
        // check if end of data
        auto nextVal = inStream.read();
        if (nextVal.strobe == 0) {
            outValue.strobe = 0;
            outStream << outValue;
            break;
        }
    // fill buffer and present_window
    lz_fill_present_win:
        while (iIdx < MATCH_LEN - 1) {
#pragma HLS PIPELINE II = 1
            inVal = nextVal;
            nextVal = inStream.read();
            present_window[++iIdx] = inVal.data[0];
        }
    // assuming that, at least bytes more than LEFT_BYTES will be present at the input
    lz_fill_circular_buf:
        for (uint16_t i = 0; i < LEFT_BYTES; ++i) {
#pragma HLS PIPELINE II = 1
            inVal = nextVal;
            nextVal = inStream.read();
            lclBufStream << inVal.data[0];
        }
        // lz_compress main
        outValue.strobe = 1;

    lz_compress:
        for (; nextVal.strobe != 0; ++iIdx) {
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = dict inter false
#endif
            uint32_t currIdx = (iIdx + (relativeNumBlocks * MAX_INPUT_SIZE)) - MATCH_LEN + 1;
            // read from input stream into circular buffer
            auto inValue = lclBufStream.read(); // pop latest value from FIFO
            lclBufStream << nextVal.data[0];    // push latest read value to FIFO
            nextVal = inStream.read();          // read next value from input stream

            // shift present window and load next value
            for (uint8_t m = 0; m < MATCH_LEN - 1; m++) {
#pragma HLS UNROLL
                present_window[m] = present_window[m + 1];
            }

            present_window[MATCH_LEN - 1] = inValue;

            // Calculate Hash Value
            uint32_t hash = 0;
            if (MIN_MATCH == 3) {
                hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^
                       (present_window[0] << 1) ^ (present_window[1]);
            } else {
                hash = (present_window[0] << 4) ^ (present_window[1] << 3) ^ (present_window[2] << 2) ^
                       (present_window[3]);
            }

            // Dictionary Lookup
            uintDictV_t dictReadValue = dict[hash];
            uintDictV_t dictWriteValue = dictReadValue << c_dictEleWidth;
            for (int m = 0; m < MATCH_LEN; m++) {
#pragma HLS UNROLL
                dictWriteValue.range((m + 1) * 8 - 1, m * 8) = present_window[m];
            }
            dictWriteValue.range(c_dictEleWidth - 1, MATCH_LEN * 8) = currIdx;
            // Dictionary Update
            dict[hash] = dictWriteValue;

            // Match search and Filtering
            // Comp dict pick
            uint8_t match_length = 0;
            uint32_t match_offset = 0;
            for (int l = 0; l < MATCH_LEVEL; l++) {
                uint8_t len = 0;
                bool done = 0;
                uintDict_t compareWith = dictReadValue.range((l + 1) * c_dictEleWidth - 1, l * c_dictEleWidth);
                uint32_t compareIdx = compareWith.range(c_dictEleWidth - 1, MATCH_LEN * 8);
                for (uint8_t m = 0; m < MATCH_LEN; m++) {
                    if (present_window[m] == compareWith.range((m + 1) * 8 - 1, m * 8) && !done) {
                        len++;
                    } else {
                        done = 1;
                    }
                }
                if ((len >= MIN_MATCH) && (currIdx > compareIdx) && ((currIdx - compareIdx) < LZ_MAX_OFFSET_LIMIT) &&
                    ((currIdx - compareIdx - 1) >= MIN_OFFSET) &&
                    (compareIdx >= (relativeNumBlocks * MAX_INPUT_SIZE))) {
                    if ((len == 3) && ((currIdx - compareIdx - 1) > 4096)) {
                        len = 0;
                    }
                } else {
                    len = 0;
                }
                if (len > match_length) {
                    match_length = len;
                    match_offset = currIdx - compareIdx - 1;
                }
            }
            outValue.data[0].range(7, 0) = present_window[0];
            outValue.data[0].range(15, 8) = match_length;
            outValue.data[0].range(31, 16) = match_offset;
            outStream << outValue;
        }

        outValue.data[0] = 0;
    lz_compress_leftover:
        for (uint8_t m = 1; m < MATCH_LEN; ++m) {
#pragma HLS PIPELINE II = 1
            outValue.data[0].range(7, 0) = present_window[m];
            outStream << outValue;
        }
    lz_left_bytes:
        for (uint16_t l = 0; l < LEFT_BYTES; ++l) {
#pragma HLS PIPELINE II = 1
            outValue.data[0].range(7, 0) = lclBufStream.read();
            outStream << outValue;
        }

        // once relativeInSize becomes 8MB set the flag to true
        resetDictFlag = ((relativeNumBlocks * MAX_INPUT_SIZE) >= (totalDictSize)) ? true : false;
        // end of block
        outValue.strobe = 0;
        outStream << outValue;
    }
}

// Content of called function
void zstdLz77DivideStream(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                          hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                          hls::stream<bool>& rleFlagStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                          hls::stream<FREQ_DT>& litCntStream) {
    // lz77 encoder states
    enum LZ77EncoderStates { WRITE_LITERAL, WRITE_OFFSET0, WRITE_OFFSET1 };
    IntVectorStream_dt<8, 1> outLitVal;
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outLitFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outSeqFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> metaVal;
    metaVal.strobe = 1;
    // frequency buffers
    FREQ_DT literal_freq[c_maxLitV + 1];
    FREQ_DT litlen_freq[c_maxCodeLL + 1];
    FREQ_DT matlen_freq[c_maxCodeML + 1];
    FREQ_DT offset_freq[c_maxCodeOF + 1];
#pragma HLS BIND_STORAGE variable = literal_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = litlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = matlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = offset_freq type = ram_2p impl = lutram

    bool last_block = false;
    bool just_started = true;
    int blk_n = 0;

    while (!last_block) {
        // iterate over multiple blocks in a file
        ++blk_n;
        enum LZ77EncoderStates next_state = WRITE_LITERAL;
        FREQ_DT seqCnt = 0;

        { // Initialize frequencies memory
#pragma HLS LOOP_MERGE force
        lit_freq_init:
            for (uint16_t i = 0; i < c_maxLitV + 1; i++) {
                literal_freq[i] = 0;
            }
        ll_freq_init:
            for (uint16_t i = 0; i < c_maxCodeLL + 1; i++) {
                litlen_freq[i] = 0;
            }
        ml_freq_init:
            for (uint16_t i = 0; i < c_maxCodeML + 1; i++) {
                matlen_freq[i] = 0;
            }
        of_freq_init:
            for (uint16_t i = 0; i < c_maxCodeOF + 1; i++) {
                offset_freq[i] = 0;
            }
        }
        uint8_t tCh = 0;
        uint8_t tLen = 0;
        FREQ_DT litCount = 0;
        FREQ_DT litTotal = 0;
        // set output data to be of valid length
        outLitVal.strobe = 1;
        outSeqVal.strobe = 1;
        uint8_t cLit = 0;
        bool fv = true;
        bool isRLE = true;
    zstd_lz77_divide:
        while (true) {
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = literal_freq inter false
#pragma HLS dependence variable = litlen_freq inter false
#pragma HLS dependence variable = matlen_freq inter false
#pragma HLS dependence variable = offset_freq inter false
#endif
            // read value from stream
            auto encodedValue = inStream.read();
            if (encodedValue.strobe == 0) {
                last_block = just_started;
                just_started = true;
                break;
            }
            just_started = false;
            tCh = encodedValue.data[0].range(7, 0);
            tLen = encodedValue.data[0].range(15, 8);
            uint16_t tOffset = encodedValue.data[0].range(31, 16) + 3 + 1;

            if (tLen) {
                // if match length present, get sequence codes
                uint8_t llc = getLLCode<16>((ap_uint<16>)litCount);
                uint8_t mlc = getMLCode<8>((ap_uint<8>)(tLen - MIN_MATCH));
                uint8_t ofc = bitsUsed31((uint32_t)tOffset);
                // reset code frequencies
                litlen_freq[llc]++;
                matlen_freq[mlc]++;
                offset_freq[ofc]++;
                // write sequence
                outSeqVal.data[0].litlen = litCount;
                outSeqVal.data[0].matlen = tLen; // - MIN_MATCH;
                outSeqVal.data[0].offset = tOffset;
                seqStream << outSeqVal;
                litCount = 0;
                ++seqCnt;
            } else {
                // store first literal to check for RLE block
                if (fv) {
                    cLit = tCh;
                } else {
                    if (cLit != tCh) isRLE = false;
                }
                fv = false;
                // increment literal count
                literal_freq[tCh]++;
                ++litCount;
                ++litTotal;
                // write literal
                outLitVal.data[0] = tCh;
                litStream << outLitVal;
            }
        }
        if (!last_block) {
            isRLE = (isRLE && cLit == 0);
            if (isRLE) {
                // printf("RLE literals\n");
                literal_freq[3]++;
                outLitVal.data[0] = 3;
                litStream << outLitVal;
            }
            rleFlagStream << isRLE;
        }
        // fix for zero sequences
        if (!last_block && seqCnt == 0) {
            outSeqVal.data[0].litlen = 0;
            outSeqVal.data[0].matlen = 0;
            outSeqVal.data[0].offset = 0;
            seqStream << outSeqVal;
        }

        // write strobe = 0
        outLitVal.strobe = 0;
        outSeqVal.strobe = 0;
        litStream << outLitVal;
        seqStream << outSeqVal;
        // write literal and distance trees
        if (!last_block) {
            metaVal.data[0] = litTotal;
            metaStream << metaVal;
            metaVal.data[0] = seqCnt;
            metaStream << metaVal;
            litCntStream << (isRLE ? (FREQ_DT)(litTotal + 1) : litTotal);
            // printf("litCount: %u, seqCnt: %u\n", (uint16_t)litTotal, (uint16_t)seqCnt);
            outLitFreqVal.strobe = 1;
            outSeqFreqVal.strobe = 1;
        write_lit_freq:
            for (ap_uint<9> i = 0; i < c_maxLitV + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outLitFreqVal.data[0] = literal_freq[i];
                litFreqStream << outLitFreqVal;
            }
            // first write total number of sequences
            outSeqFreqVal.data[0] = seqCnt;
            seqFreqStream << outSeqFreqVal;
            // write last ll, ml, of codes
            outSeqFreqVal.data[0] = getLLCode<16>(outSeqVal.data[0].litlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = getMLCode<8>(outSeqVal.data[0].matlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = bitsUsed31(outSeqVal.data[0].offset);
            seqFreqStream << outSeqFreqVal;
        write_lln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeLL + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = litlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_ofs_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeOF + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = offset_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_mln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeML + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = matlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        }
    }
    // eos needed only to indicated end of block
    outLitFreqVal.strobe = 0;
    outSeqFreqVal.strobe = 0;
    metaVal.strobe = 0;
    litFreqStream << outLitFreqVal;
    seqFreqStream << outSeqFreqVal;
    metaStream << metaVal;
}

// Content of called function
void downSizeLitlen(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outSeqStream) {
    // Downsize Literal length to 8-bits
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> outSeqVal;
    bool done = false;
dsz_ll_outer:
    while (!done) {
        auto inSeqVal = inSeqStream.read();
        bool dszDone = (inSeqVal.strobe == 0);
        done = dszDone;
        auto litLen = inSeqVal.data[0].litlen;
        outSeqVal.strobe = 1;
    dsz_litlen:
        while (!dszDone) {
#pragma HLS PIPELINE II = 1
            if (litLen > 255) {
                outSeqVal.data[0].setLitlen(255);
                outSeqVal.data[0].setMatlen(0);
                outSeqVal.data[0].setOffset(0);
                litLen -= 255;
            } else {
                outSeqVal.data[0].setLitlen(litLen.range(7, 0));
                outSeqVal.data[0].setMatlen(inSeqVal.data[0].matlen);
                outSeqVal.data[0].setOffset(inSeqVal.data[0].offset);

                // read next input sequence
                inSeqVal = inSeqStream.read();
                litLen = inSeqVal.data[0].litlen;
                dszDone = (inSeqVal.strobe == 0);
            }
            // write output sequence
            outSeqStream << outSeqVal;
        }
        // End of block/file
        outSeqVal.strobe = 0;
        outSeqStream << outSeqVal;
    }
}

// Content of called function
void lzBooster(hls::stream<compressd_dt>& inStream, hls::stream<compressd_dt>& outStream, uint32_t input_size) {
    if (input_size == 0) return;
    uint8_t local_mem[BOOSTER_OFFSET_WINDOW];
    uint32_t match_loc = 0;
    uint32_t match_len = 0;
    compressd_dt outValue;
    compressd_dt outStreamValue;
    bool matchFlag = false;
    bool outFlag = false;
    bool boostFlag = false;
    uint16_t skip_len = 0;
    uint8_t nextMatchCh = local_mem[match_loc % BOOSTER_OFFSET_WINDOW];

lz_booster:
    for (uint32_t i = 0; i < (input_size - LEFT_BYTES); i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = local_mem inter false
        compressd_dt inValue = inStream.read();
        uint8_t tCh = inValue.range(7, 0);
        uint8_t tLen = inValue.range(15, 8);
        uint16_t tOffset = inValue.range(31, 16);
        if (tOffset < BOOSTER_OFFSET_WINDOW) {
            boostFlag = true;
        } else {
            boostFlag = false;
        }
        uint8_t match_ch = nextMatchCh;
        local_mem[i % BOOSTER_OFFSET_WINDOW] = tCh;
        outFlag = false;

        if (skip_len) {
            skip_len--;
        } else if (matchFlag && (match_len < MAX_MATCH_LEN) && (tCh == match_ch)) {
            match_len++;
            match_loc++;
            outValue.range(15, 8) = match_len;
        } else {
            match_len = 1;
            match_loc = i - tOffset;
            if (i) outFlag = true;
            outStreamValue = outValue;
            outValue = inValue;
            if (tLen) {
                if (boostFlag) {
                    matchFlag = true;
                    skip_len = 0;
                } else {
                    matchFlag = false;
                    skip_len = tLen - 1;
                }
            } else {
                matchFlag = false;
            }
        }
        nextMatchCh = local_mem[match_loc % BOOSTER_OFFSET_WINDOW];
        if (outFlag) outStream << outStreamValue;
    }
    outStream << outValue;
lz_booster_left_bytes:
    for (uint32_t i = 0; i < LEFT_BYTES; i++) {
#pragma HLS PIPELINE off
        outStream << inStream.read();
    }
}

// Content of called function
void reverseSeq(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& inSeqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outReversedSeqStream) {
    // reverse sequences
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszReversedSeqStream("dszReversedSeqStream");
#pragma HLS STREAM variable = dszReversedSeqStream depth = 16

#pragma HLS DATAFLOW
    // reverse sequences
    details::reverseSeqIntl<BLOCK_SIZE, MAX_FREQ_DWIDTH>(inSeqStream, dszReversedSeqStream);
    // upsize reversed sequences to MAX_FREQ_DWIDTH-bit literal length
    details::upSizeLitlen<MAX_FREQ_DWIDTH, MIN_MATCH>(dszReversedSeqStream, outReversedSeqStream);
}

// Content of called function
void reverseSeqIntl(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& seqStream,
                    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outReversedSeqStream) {
    constexpr uint32_t c_seqMemSize = BLOCK_SIZE / 4;
    // sequence buffer
    SequencePack<MAX_FREQ_DWIDTH, 8> seqBuffer[c_seqMemSize];
#pragma HLS BIND_STORAGE variable = seqBuffer type = ram_t2p impl = bram

    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> outSeqV;
    bool done = false;
    bool blockDone = true;
    // operation modes
    bool streamMode = 0;
    bool bufferMode = 1;

    uint16_t memReadBegin[2];                               // starting index for BRAM read
    constexpr int16_t memReadLimit[2] = {-1, c_seqMemSize}; // last index for buffer read
#pragma HLS ARRAY_PARTITION variable = memReadBegin complete
#pragma HLS ARRAY_PARTITION variable = memReadLimit complete
    uint16_t wIdx = 0;
    int16_t rIdx = 0; // may become negative for writing strobe 0
    int8_t wInc = 1, rInc = 1;
    uint8_t rsi = 0, wsi = 0;
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> inSeq;
    outSeqV.strobe = 1;
// sequence read and reverse
reverse_seq_stream:
    while (bufferMode || streamMode) {
#pragma HLS PIPELINE II = 1 rewind
        if (bufferMode) {
            // Write to internal memory
            inSeq = seqStream.read();
            seqBuffer[wIdx].setData(inSeq.data[0].getData());
            // check mode
            if (inSeq.strobe == 0) {
                done = blockDone;
                blockDone = true;
                // Enable memory read/stream mode
                if (streamMode == 0 && !done) {
                    streamMode = 1;
                    rIdx = wIdx - wInc;
                }
                // set mem read begin index
                memReadBegin[wsi] = wIdx - wInc; // since an extra increment has been done here
                // change increment direction
                wInc = (~wInc) + 1;  // flip 1 and -1
                wsi = (wsi + 1) & 1; // flip 1 and 0
                // post increment check if bufferMode to be continued/paused/stopped
                if (done || (wsi == rsi)) bufferMode = 0;
                // set stream mode's last mem index for even and odd stream indices
                wIdx = (uint16_t)(memReadLimit[wsi] + wInc);
                continue;
            } else {
                blockDone = false;
                // directional increment
                wIdx += wInc;
            }
        }
        if (streamMode) {
            // update stream mode state
            if (rIdx == memReadLimit[rsi]) {
                // write end of block indication as strobe 0
                outSeqV.strobe = 0;
                // in case bufferMode pauses due to finishing earlier
                if (bufferMode == 0) bufferMode = (wsi == rsi && !done);
                rsi = (rsi + 1) & 1;
                rIdx = memReadBegin[rsi];
                rInc = (~rInc) + 1; // flip 1 and -1
                // either previous streamMode ended quicker than next bufferMode or streamCnt reached
                if (done || (wsi == rsi)) streamMode = 0;
            } else {
                // Read from internal memory to output stream
                outSeqV.data[0].setData(seqBuffer[rIdx].getData());
                // directional decrement
                rIdx -= rInc;
                outSeqV.strobe = 1;
            }
            // write data or strobe 0
            outReversedSeqStream << outSeqV;
        }
    }
    // file end indication
    outSeqV.strobe = 0;
    outReversedSeqStream << outSeqV;
}

// Content of called function
void upSizeLitlen(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& inSeqStream,
                  hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outSeqStream) {
    // Upsize litlen to MAX_FREQ_DWIDTH-bits from 8-bits in reversed sequences stream
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    bool done = false;
upsz_ll_outer:
    while (!done) {
        auto inSeqVal = inSeqStream.read();
        bool uszDone = (inSeqVal.strobe == 0);
        done = uszDone;
        // store current sequence
        outSeqVal.data[0].litlen = inSeqVal.data[0].getLitlen();
        outSeqVal.data[0].offset = inSeqVal.data[0].getOffset();
        // check for noSeq condition
        if (inSeqVal.data[0].getLitlen() == 0 && inSeqVal.data[0].getMatlen() == 0 &&
            inSeqVal.data[0].getOffset() == 0) {
            outSeqVal.data[0].matlen = 0;
        } else {
            outSeqVal.data[0].matlen = inSeqVal.data[0].getMatlen() - MIN_MATCH;
        }
        outSeqVal.strobe = 1;
    upsz_litlen:
        while (!uszDone) {
#pragma HLS PIPELINE II = 1
            inSeqVal = inSeqStream.read();
            uszDone = (inSeqVal.strobe == 0);
            if (inSeqVal.data[0].getMatlen() == 0 && inSeqVal.data[0].getOffset() == 0 && !uszDone) {
                outSeqVal.data[0].litlen = outSeqVal.data[0].litlen + inSeqVal.data[0].getLitlen();
            } else { // if regular or last sequence
                outSeqStream << outSeqVal;
                // update out sequence values with current values
                outSeqVal.data[0].litlen = inSeqVal.data[0].getLitlen();
                outSeqVal.data[0].matlen = inSeqVal.data[0].getMatlen() - MIN_MATCH;
                outSeqVal.data[0].offset = inSeqVal.data[0].getOffset();
            }
        }
        // end of block/file
        outSeqVal.strobe = 0;
        outSeqStream << outSeqVal;
    }
}

// Content of called function
void frequencySequencer(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& wghtFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& freqStream) {
    // sequence input frequencies into single output stream
    constexpr uint16_t c_limits[4] = {c_maxCodeLL + 1, c_maxCodeOF + 1, c_maxCodeML + 1, c_maxZstdHfBits + 1};
    const uint8_t c_hfIdx = 3;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outfreq;
    while (true) {
        outfreq = seqFreqStream.read();
        if (outfreq.strobe == 0) break; // will come only at the end of all data
        // first value is sequences count
        freqStream << outfreq;
    // next three values are last ll, ml, of codes
    write_last_seq:
        for (uint8_t i = 0; i < 3; ++i) {
#pragma HLS PIPELINE II = 1
            outfreq = seqFreqStream.read();
            freqStream << outfreq;
        }
    write_bl_ll_ml_of:
        for (uint8_t fIdx = 0; fIdx < 4; ++fIdx) {
            auto llim = c_limits[fIdx];
            bool isBlen = (c_hfIdx == fIdx);
            if (isBlen) {
                // maxVal from literals
                outfreq = wghtFreqStream.read();
                freqStream << outfreq;
            }
            // first send the size, followed by data
            outfreq.data[0] = llim;
            freqStream << outfreq;
        write_inp_freq:
            for (uint16_t i = 0; i < llim; ++i) {
#pragma HLS PIPELINE II = 1
                if (isBlen) {
                    outfreq = wghtFreqStream.read();
                } else {
                    outfreq = seqFreqStream.read();
                }
                freqStream << outfreq;
            }
            // dump strobe 0
            if (isBlen) wghtFreqStream.read();
        }
    }
    // dump strobe 0
    wghtFreqStream.read();
    outfreq.data[0] = 0;
    outfreq.strobe = 0;
    freqStream << outfreq;
}

// Content of called function
void serializeLiterals(hls::stream<ap_uint<68> > litUpsizedStream[CORE_COUNT],
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> > litCntStream[CORE_COUNT],
                       hls::stream<ap_uint<68> >& outLitUpsizedStream,
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outLitCntStream) {
    bool allDone = false;
    ap_uint<68> litVal;
    uint8_t cIdx = 0;
srlz_lit_all_data:
    while (!allDone) {
    // get upsized literals from each core
    srlz_lit_all_cores:
        for (cIdx = 0; cIdx < CORE_COUNT; ++cIdx) {
            litVal = litUpsizedStream[cIdx].read();
            if (litVal.range(3, 0) == 0) { // break if first read is zero
                allDone = true;
                break;
            }
            // write first word (used for continuation or termination)
            outLitUpsizedStream << litVal;
            // get literal count
            auto litCnt = litCntStream[cIdx].read();
            outLitCntStream << litCnt;
        srlz_lit_loop:
            for (litVal = litUpsizedStream[cIdx].read(); litVal.range(3, 0) > 0;
                 litVal = litUpsizedStream[cIdx].read()) {
#pragma HLS PIPELINE II = 1
                outLitUpsizedStream << litVal;
            }
            // end of block
            outLitUpsizedStream << 0;
        }
    }
    // write strobe 0 to output
    outLitUpsizedStream << 0;
// dump strobe 0
srlz_lit_eos_dump:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
#pragma HLS PIPELINE II = 1
        if (i != cIdx) litUpsizedStream[i].read();
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
void streamCmpStrdFrame(hls::stream<ap_uint<68> >& inRawBStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& inCmpBStream,
                        hls::stream<ap_uint<2> >& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    IntVectorStream_dt<8, 8> outVal;
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
    // unsigned t = 0;
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
            for (auto rbVal = inRawBStream.read(); rbVal.range(3, 0) > 0; rbVal = inRawBStream.read()) {
#pragma HLS PIPELINE II = 1
                for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                    outVal.data[i] = rbVal.range((i * 8) + 11, (i * 8) + 4);
                }
                outVal.strobe = rbVal.range(3, 0);
                outStream << outVal;
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
void inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                      hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                      hls::stream<ap_uint<68> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    constexpr uint16_t c_idataStreamDepth = 256 / CORE_COUNT;
    // Internal streams
    hls::stream<IntVectorStream_dt<8, 8> > intmDataStream("intmDataStream");
    hls::stream<bool> rawBlockFlagStream("rawBlockFlagStream");

#pragma HLS STREAM variable = intmDataStream depth = c_idataStreamDepth
#pragma HLS STREAM variable = rawBlockFlagStream depth = 32

#pragma HLS dataflow
    xf::compression::details::inputBufferMinBlock<BLOCK_SIZE, MIN_BLK_SIZE>(inStream, rawBlockFlagStream,
                                                                            intmDataStream);

    xf::compression::details::__inputDistributer<BLOCK_SIZE, CORE_COUNT>(intmDataStream, rawBlockFlagStream, outStream,
                                                                         outStrdStream, blockMetaStream);
}

// Content of called function
void inputBufferMinBlock(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                         hls::stream<bool>& rawBlockFlagStream,
                         hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    // write data and indicate if it should be raw block or not
    bool not_done = true;
    bool rawFlagNotSent = true;
    IntVectorStream_dt<8, 8> inVal;
stream_data:
    while (not_done) {
        // read data size in bytes
        uint32_t dataSize = 0;
        inVal.strobe = 8;
        rawFlagNotSent = true;
    send_in_block:
        while (inVal.strobe > 0 && dataSize < BLOCK_SIZE) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            if (inVal.strobe > 0) {
                outStream << inVal;
                dataSize += inVal.strobe;
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
void __inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                        hls::stream<ap_uint<68> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 8> inVal;
    ap_uint<68> rawVal;
    ap_uint<68> outVal;
    uint8_t coreIdx = 0;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            // assign values
            rawVal.range(3, 0) = inVal.strobe;
            outVal.range(3, 0) = inVal.strobe;
            for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                rawVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
                outVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
            }
            // write to output streams
            if (!isRawBlock) outStream[coreIdx] << outVal;
            outStrdStream << rawVal;
            dataSize += inVal.strobe;
        }
        // End of block/file
        if (!isRawBlock) outStream[coreIdx] << 0;
        outStrdStream << 0;
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
        ++coreIdx;
        coreIdx = ((coreIdx < CORE_COUNT) ? coreIdx : 0);
        // coreIdx = (coreIdx + 1) % CORE_COUNT; // increment within limits
    }
// terminate all lz77 blocks
terminate_blocks:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
        if (i != coreIdx) outStream[i] << 0; // "coreIdx" lz77 already terminated
    }
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
void serializeLZData(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > seqStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > litFreqStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > seqFreqStream[CORE_COUNT],
                     hls::stream<bool> rleFlagStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lzMetaStream[CORE_COUNT],
                     hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outSeqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outLitFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outSeqFreqStream,
                     hls::stream<bool>& outRleFlagStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outLzMetaStream) {
    constexpr uint8_t c_seqFreqCnt = (details::c_maxCodeLL + details::c_maxCodeOF + details::c_maxCodeML) + 7;
    bool allDone = false;
    uint8_t cIdx = 0;
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> seqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> litFreqVal, seqFreqVal, lzMetaVal;
    bool rleFlag;

srlz_lz_all_data:
    while (!allDone) {
    // get lz data from all cores and write to individual output streams
    srlz_lz_data_all_cores:
        for (cIdx = 0; cIdx < CORE_COUNT; ++cIdx) {
            seqVal = seqStream[cIdx].read();
            if (seqVal.strobe == 0) { // break if first read is zero
                allDone = true;
                break;
            } else {
#pragma HLS DATAFLOW
                // write sequences
                {
                srlz_lz_seq_loop:
                    for (; seqVal.strobe > 0; seqVal = seqStream[cIdx].read()) {
#pragma HLS PIPELINE II = 1
                        outSeqStream << seqVal;
                    }
                    // end of block, strobe 0
                    outSeqStream << seqVal;
                }
                // write rle flag
                {
                    rleFlag = rleFlagStream[cIdx].read();
                    outRleFlagStream << rleFlag;
                }
            // write lz meta values
            srlz_lz_meta_loop:
                for (uint8_t m = 0; m < 2; ++m) {
#pragma HLS PIPELINE II = 1
                    lzMetaVal = lzMetaStream[cIdx].read();
                    outLzMetaStream << lzMetaVal;
                }
            // write literal frequencies
            srlz_lz_lit_freq_loop:
                for (uint16_t k = 0; k < details::c_maxLitV + 1; ++k) {
#pragma HLS PIPELINE II = 1
                    litFreqVal = litFreqStream[cIdx].read();
                    outLitFreqStream << litFreqVal;
                }
            // write sequence frequencies
            srlz_lz_seq_freq_loop:
                for (uint8_t s = 0; s < c_seqFreqCnt; ++s) {
#pragma HLS PIPELINE II = 1
                    seqFreqVal = seqFreqStream[cIdx].read();
                    outSeqFreqStream << seqFreqVal;
                }
            }
        }
    }
    // write strobe 0 to output
    seqVal.strobe = 0;
    litFreqVal.strobe = 0;
    seqFreqVal.strobe = 0;
    lzMetaVal.strobe = 0;
    outSeqStream << seqVal;
    outLitFreqStream << litFreqVal;
    outSeqFreqStream << seqFreqVal;
    outLzMetaStream << lzMetaVal;
// dump strobe 0
srlz_lz_eos_dump:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
#pragma HLS PIPELINE II = 1
        if (i != cIdx) {
            seqStream[i].read();
        }
        // read eos from all other streams
        litFreqStream[i].read();
        seqFreqStream[i].read();
        lzMetaStream[i].read();
    }
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
void reverseLitQuadStreams(hls::stream<ap_uint<68> >& inLitStream,
                           hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream,
                           hls::stream<ap_uint<68> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    constexpr uint16_t c_litBufSize = (BLOCK_SIZE / 32);
    // storing only 1/4th of the block size, 8-bytes a time
    // literal buffer
    ap_uint<64> litBuffer[c_litBufSize];
#pragma HLS BIND_STORAGE variable = litBuffer type = ram_t2p impl = bram
    ap_uint<64> outLit;
    ap_uint<68> outVal;

pre_proc_lit_main:
    while (true) {
        bool rdLitFlag = false;
        auto inVal = inLitStream.read();
        if (inVal.range(3, 0) == 0) break;
        uint8_t streamCnt = 1;
        uint16_t streamSize[5] = {0, 0, 0, 0, 0}; // 1 dummy entry
#pragma HLS ARRAY_PARTITION variable = streamSize complete
        ap_uint<MAX_FREQ_DWIDTH> litCnt = litCntStream.read();
        // get out stream count and sizes
        if (litCnt > 255) {
            streamCnt = 4;
            streamSize[0] = 1 + (litCnt - 1) / 4;
            streamSize[1] = streamSize[0];
            streamSize[2] = streamSize[0];
            streamSize[3] = litCnt - (streamSize[0] * 3);
        } else {
            streamSize[0] = litCnt;
            streamSize[3] = litCnt;
        }
        outVal.range(3, 0) = 1;
        // first send the stream count
        outVal.range(11, 4) = streamCnt;
        outReversedLitStream << outVal;
    write_stream_sizes:
        for (uint8_t k = 0; k < streamCnt; ++k) {
#pragma HLS UNROLL
            outLit.range(15 + (k * 16), k * 16) = streamSize[k];
        }
        outVal.range(67, 4) = outLit;
        outReversedLitStream << outVal;
        // indexes
        uint16_t litRcnt = 0, litWcnt = 0;
        uint16_t wIdx = 1;
        uint16_t rIdx = 0;
        // write already read value into buffer
        litBuffer[0] = inVal.range(67, 4);
        ap_uint<64> prevWord = 0;
        uint8_t extBytesRead = 0;
        int8_t rwInc = 1;
        litWcnt = inVal.range(3, 0);
    rev_lit_loop_1to5:
        for (uint8_t si = 0; si < streamCnt + 1; ++si) {
            if (extBytesRead) {
                // write first byte to buffer
                litBuffer[wIdx] = prevWord;
                wIdx += rwInc;
                litWcnt = extBytesRead;
                // write first output for previous sub-stream
                outVal.range(3, 0) = 8 - extBytesRead;
                outLit = litBuffer[rIdx];
            // assign value in reverse byte order
            assign_outVal_1:
                for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                    uint8_t rK = 7 - k;
                    outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                }
                outReversedLitStream << outVal;
                rIdx += rwInc;
                litRcnt = outVal.range(3, 0);
            }
        rev_lit_loop_overlap:
            while ((si < streamCnt && litWcnt < streamSize[si]) || (si > 0 && litRcnt < streamSize[si - 1])) {
#pragma HLS PIPELINE II = 1
                // run till one of the conditions is true
                // buffer sub-stream
                if (si < streamCnt && litWcnt < streamSize[si]) {
                    inVal = inLitStream.read();
                    litBuffer[wIdx] = inVal.range(67, 4);
                    prevWord = inVal.range(67, 4);
                    litWcnt += 8; // to handle last stream size not aligned to 8 bytes case
                    wIdx += rwInc;
                }
                // read-out buffer data in reverse after first sub-stream is buffered
                if (si > 0 && litRcnt < streamSize[si - 1]) {
                    outLit = litBuffer[rIdx];
                    rIdx += rwInc;
                // assign value in reverse byte order
                assign_outVal_2:
                    for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                        uint8_t rK = 7 - k;
                        outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                    }
                    outVal.range(3, 0) = 8;
                    outReversedLitStream << outVal;
                    litRcnt += 8;
                }
            }
            // printf("Written %u bytes\n", wb);
            // get extra bytes for currently buffered stream
            extBytesRead = litWcnt - streamSize[si];
            // re-initialize literal index in sub-stream
            litWcnt = 0;
            litRcnt = 0;
            // manage direction in memory access
            // both indices move in same direction
            rwInc = (~rwInc) + 1; // flip +1/-1
            // manage memory access indices
            if ((uint8_t)(si & 1) != 0) { // odd, works only after type-casting to int types
                rIdx = wIdx + 1;
                wIdx = 0;
            } else { // even
                rIdx = wIdx - 1;
                wIdx = c_litBufSize - 1;
            }
        }
        // dump strobe 0 from input
        inVal = inLitStream.read();
        // end of block indicator not needed, since size is also sent
    }
    // end of data
    outVal.range(3, 0) = 0;
    outReversedLitStream << outVal;
}

// Content of called function
void zstdCompressionMeta(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                         hls::stream<ap_uint<2> >& strdBlockFlagStream,
                         hls::stream<DT>& outMetaStream) {
    uint8_t i = 0;
    DT litCnt = 0;
    ap_uint<2> stbFVal = 1; // <stb Flag 1-bit><strobe 1-bit>
gen_meta_loop:
    for (auto inMetaVal = metaStream.read(); inMetaVal.strobe > 0; inMetaVal = metaStream.read()) {
#pragma HLS PIPELINE off
        litCnt = inMetaVal.data[0];
        outMetaStream << litCnt;
        if (i == 0) {
            stbFVal.range(1, 1) = (ap_int<1>)(litCnt > BLOCK_SIZE - 2048);
            strdBlockFlagStream << stbFVal;
        }
        i = (i + 1) & 1;
    }
    // end of all data
    stbFVal = 0;
    strdBlockFlagStream << stbFVal;
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