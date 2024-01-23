void reverseSeq(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outReversedSeqStream) {
    // reverse sequences
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszSeqStream("dszSeqStream");
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszReversedSeqStream("dszReversedSeqStream");
#pragma HLS STREAM variable = dszSeqStream depth = 16
#pragma HLS STREAM variable = dszReversedSeqStream depth = 16

#pragma HLS DATAFLOW
    // MAX_FREQ_DWIDTH-bit literal length to 8-bit
    details::downSizeLitlen<MAX_FREQ_DWIDTH>(inSeqStream, dszSeqStream);
    // reverse sequences
    details::reverseSeqIntl<BLOCK_SIZE, MAX_FREQ_DWIDTH>(dszSeqStream, dszReversedSeqStream);
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