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