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