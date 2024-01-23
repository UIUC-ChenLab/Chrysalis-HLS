void byteGen(bitBufferTypeLL& _bitbuffer,
             ap_uint<6>& bits_cntr,
             ap_uint<16>* codeOffsets,
             ap_uint<9>* bl1Codes,
             ap_uint<9>* bl2Codes,
             ap_uint<9>* bl3Codes,
             ap_uint<9>* bl4Codes,
             ap_uint<9>* bl5Codes,
             ap_uint<9>* bl6Codes,
             ap_uint<9>* bl7Codes,
             ap_uint<5>* lens,
             hls::stream<bool>& inEos,
             hls::stream<ap_uint<16> >& inStream,
             ap_uint<9> nlen,
             ap_uint<9> ndist,
             bool& done) {
    loadBitStreamLL(_bitbuffer, bits_cntr, inStream, inEos, done);
    uint8_t copy = 0;
    uint8_t extra_copy = 0;
    uint8_t nNum = 0;
    ap_uint<7> validCodeOffset;
    ap_uint<5> symbol[8];
#pragma HLS ARRAY_PARTITION variable = symbol dim = 1 complete
    uint16_t len = 0;
    uint16_t dynamic_curInSize = 0;
    bitBufferTypeLL bitbuffer[2];
    bool isExtra = false;
    ap_uint<3> extra = 0;
    ap_uint<4> current_bits = 0;
bytegen:
    while ((dynamic_curInSize < nlen + ndist) || (copy != 0)) {
#pragma HLS PIPELINE II = 1
        for (ap_uint<5> i = 0; i < 7; i++) {
#pragma HLS UNROLL
            bool val = (_bitbuffer.range(0, i) >= codeOffsets[i]) ? 1 : 0;
            if (val) {
                current_bits = i + 1;
            }
        }
        symbol[1] = bl1Codes[_bitbuffer.range(0, 0)];
        symbol[2] = bl2Codes[_bitbuffer.range(0, 1)];
        symbol[3] = bl3Codes[_bitbuffer.range(0, 2)];
        symbol[4] = bl4Codes[_bitbuffer.range(0, 3)];
        symbol[5] = bl5Codes[_bitbuffer.range(0, 4)];
        symbol[6] = bl6Codes[_bitbuffer.range(0, 5)];
        symbol[7] = bl7Codes[_bitbuffer.range(0, 6)];

        auto current_val = symbol[current_bits];
        bitbuffer[0] = _bitbuffer >> current_bits;
        bitbuffer[1] = _bitbuffer >> extra;
        extra_copy = _bitbuffer & ((1 << extra) - 1);

        if (isExtra) {
            isExtra = false;
            copy += extra_copy;
            _bitbuffer = bitbuffer[1];
            bits_cntr -= extra;
        } else if (copy == 0) {
            if (current_val < 16) {
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                len = current_val;
                copy = 1;
            } else if (current_val == 16) {
                copy = 3;                  // use 2 bits
                _bitbuffer = bitbuffer[0]; // dump 2 bits
                bits_cntr -= current_bits; // update bits_cntr
                extra = 2;
                isExtra = true;
            } else if (current_val == 17) {
                len = 0;
                copy = 3; // use 3 bits
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                extra = 3;
                isExtra = true;
            } else {
                len = 0;
                copy = 11; // use 7 bits
                _bitbuffer = bitbuffer[0];
                bits_cntr -= current_bits;
                extra = 7;
                isExtra = true;
            }
        }

        if (copy != 0) {
            lens[dynamic_curInSize++] = len;
            copy -= 1;
        }
        if (bits_cntr < 16 && !done) {
            uint16_t tmp_data = inStream.read();
            _bitbuffer += (bitBufferTypeLL)(tmp_data) << bits_cntr;
            done = inEos.read();
            bits_cntr += 16;
        }
    }
}

// Content of called function
void loadBitStreamLL(bitBufferTypeLL& bitbuffer,
                     ap_uint<6>& bits_cntr,
                     hls::stream<ap_uint<16> >& inStream,
                     hls::stream<bool>& inEos,
                     bool& done) {
    if (bits_cntr < 16 && (done == false)) {
    loadBitStream:
        uint16_t tmp_dt = (uint16_t)inStream.read();
        bitbuffer += (bitBufferTypeLL)(tmp_dt) << bits_cntr;
        done = inEos.read();
        bits_cntr += 16;
    }
}

// Content of called function
void loadBitStream(bitBufferType& bitbuffer,
                   ap_uint<6>& bits_cntr,
                   hls::stream<ap_uint<16> >& inStream,
                   hls::stream<bool>& inEos,
                   bool& done) {
#pragma HLS INLINE off
    while (bits_cntr < 32 && (done == false)) {
    loadBitStream:
        uint16_t tmp_dt = (uint16_t)inStream.read();
        bitbuffer += (bitBufferType)(tmp_dt) << bits_cntr;
        done = inEos.read();
        bits_cntr += 16;
    }
}