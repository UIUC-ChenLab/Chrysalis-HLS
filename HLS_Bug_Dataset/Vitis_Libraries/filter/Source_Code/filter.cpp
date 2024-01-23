void filter(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& inFreq,
            Symbol<MAX_FREQ_DWIDTH>* heap,
            uint16_t* heapLength,
            hls::stream<ap_uint<c_tgnSymbolBits> >& symLength,
            uint16_t i_symSize) {
    uint16_t hpLen = 0;
    ap_uint<c_tgnSymbolBits> smLen = 0;
    bool read_flag = false;
    auto curFreq = inFreq.read();
    if (curFreq.strobe == 0) {
        heapLength[0] = 0xFFFF;
        return;
    }
filter:
    for (uint16_t n = 0; n < i_symSize; ++n) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT max = 286 min = 19
        if (read_flag) curFreq = inFreq.read();
        auto cf = curFreq.data[0];
        read_flag = true;
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
    if (WRITE_MXC) symLength << smLen;
}