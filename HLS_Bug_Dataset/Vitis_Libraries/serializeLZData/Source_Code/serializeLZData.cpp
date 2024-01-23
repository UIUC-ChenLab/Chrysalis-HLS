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