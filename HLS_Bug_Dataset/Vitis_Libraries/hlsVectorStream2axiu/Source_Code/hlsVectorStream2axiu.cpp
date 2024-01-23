void hlsVectorStream2axiu(hls::stream<IntVectorStream_dt<OUT_DWIDTH, 1> >& inputStream,
                          hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outAxiStream) {
    bool allDone = false;
    bool just_started = true;
write_all_files:
    do {
        bool last = false;
    write_out_file_data:
        do {
#pragma HLS PIPELINE II = 1
            auto inVal = inputStream.read();
            if (inVal.strobe) {
                just_started = false;
            } else { // last file
                allDone = just_started;
                just_started = true;
                last = true;
                if (allDone) break;
            }
            ap_axiu<OUT_DWIDTH, 0, 0, 0> tOut;
            tOut.data = inVal.data[0];
            tOut.last = last;
            tOut.keep = -1;
            outAxiStream << tOut;
        } while (!last);
    } while (!allDone);
}