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