void bitDownSizeByte(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                     hls::stream<ap_uint<INSIZE_WIDTH> >& inSizeStream,
                     hls::stream<IntVectorStream_dt<8, OUT_WIDTH / 8> >& outStream) {
    // downsize in bits
    constexpr uint8_t c_outBytes = OUT_WIDTH / 8;
    constexpr uint8_t c_accBytes = IN_WIDTH + OUT_WIDTH;
    IntVectorStream_dt<8, c_outBytes> outVal;
    bool done = false;

bit_downsizer_outer:
    while (!done) {
        done = true;
        int16_t bitsInAcc = 0;
        ap_uint<c_accBytes> accReg = 0;
        outVal.strobe = c_outBytes; // assign const strobe
        ap_uint<OUT_WIDTH> outReg;
        ap_uint<IN_WIDTH> inVal = 0;
        auto inSizeVal = inSizeStream.read();
        bool blkDone = (inSizeVal == 0);
    bit_downsizer_main:
        while (!blkDone) {
#pragma HLS PIPELINE II = 1
            done = false;
            // read & assign input to accumulator
            if (bitsInAcc < OUT_WIDTH && inSizeVal > 0) {
                inVal = inStream.read();
                accReg += ((ap_uint<c_accBytes>)inVal << bitsInAcc);
                bitsInAcc += inSizeVal;
            }
            // write output
            if (bitsInAcc > OUT_WIDTH - 1 || (bitsInAcc > 0 && inSizeVal == 0)) {
                for (uint8_t k = 0; k < c_outBytes; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = accReg.range((k * 8) + 7, k * 8);
                }
                // get strobe
                outVal.strobe = ((bitsInAcc > OUT_WIDTH - 1) ? c_outBytes : ((bitsInAcc + 7) >> 3));
                // write output
                outStream << outVal;
                // update bitcnt and accumulator
                accReg >>= OUT_WIDTH;
                bitsInAcc -= OUT_WIDTH; // will become negative at last
            }
            // read next word size and set end flag
            if (bitsInAcc < OUT_WIDTH && inSizeVal > 0) {
                inSizeVal = inSizeStream.read();
            }
            blkDone = (inSizeVal == 0 && bitsInAcc < 1);
        }
        // end of block/data
        outVal.strobe = 0;
        outStream << outVal;
    }
}