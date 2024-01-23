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