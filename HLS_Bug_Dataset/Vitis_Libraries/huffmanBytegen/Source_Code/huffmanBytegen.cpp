uint8_t huffmanBytegen(bitBufferType& _bitbuffer,
                       ap_uint<6>& bits_cntr,
                       hls::stream<ap_uint<17> >& outStream,
                       hls::stream<bool>& inEos,
                       hls::stream<ap_uint<16> >& inStream,
                       const uint32_t* array_codes,
                       const uint32_t* array_codes_extra,
                       const uint32_t* array_codes_dist,
                       const uint32_t* array_codes_dist_extra,
                       bool& done) {
#pragma HLS INLINE
    uint16_t lit_mask = 511; // Adjusted according to 8 bit
    uint16_t dist_mask = 511;
    bitBufferType bitbuffer = details::reg<bitBufferType>(_bitbuffer);
    //    bitBufferType bitbuffer = _bitbuffer;
    ap_uint<9> lidx = bitbuffer;
    ap_uint<9> lidx1;
    ap_uint<32> current_array_val = details::reg<ap_uint<32> >(array_codes[lidx]);
    uint8_t current_op = current_array_val.range(31, 24);
    uint8_t current_bits = current_array_val.range(23, 16);
    uint16_t current_val = current_array_val.range(15, 0);
    bool is_length = true;
    ap_uint<32> tmpVal;
    uint8_t ret = 0;
    bool dist_extra = false;
    bool len_extra = false;

    bool huffDone = false;
ByteGen:
    for (; !huffDone;) {
#pragma HLS PIPELINE II = 1
        ap_uint<4> len1 = current_bits;
        ap_uint<4> len2 = 0;
        ap_uint<4> ml_op = current_op;
        uint8_t current_op1 = (current_op == 0 || current_op >= 64) ? 1 : current_op;
        ap_uint<64> bitbuffer1 = bitbuffer >> current_bits;
        ap_uint<9> bitbuffer3 = bitbuffer >> current_bits;
        lidx1 = bitbuffer1.range(current_op1 - 1, 0) + current_val;
        ap_uint<9> bitbuffer2 = bitbuffer.range(current_bits + ml_op + 8, current_bits + ml_op);
        bits_cntr -= current_bits;
        dist_extra = false;
        len_extra = false;

        if (current_op == 0) {
            tmpVal.range(8, 1) = (uint8_t)(current_val);
            tmpVal.range(16, 9) = 0xFF;
            tmpVal.range(0, 0) = 0;
            // std::cout << (char)(current_val);
            outStream << tmpVal;
            lidx = bitbuffer3;
            is_length = true;
        } else if (current_op & 16) {
            uint16_t len = (uint16_t)(current_val);
            len += (uint16_t)bitbuffer1 & ((1 << ml_op) - 1);
            len2 = ml_op;
            bits_cntr -= ml_op;
            tmpVal.range(0, 0) = 0;
            tmpVal.range(16, 1) = len;
            outStream << tmpVal;
            lidx = bitbuffer2;
            is_length = !(is_length);
        } else if ((current_op & 64) == 0) {
            if (is_length) {
                len_extra = true;
            } else {
                dist_extra = true;
            }
        } else if (current_op & 32) {
            if (is_length) {
                ret = blockStatus::PENDING;
            } else {
                ret = blockStatus::FINISH;
            }
            huffDone = true;
        }
        if ((done == true) && (bits_cntr < 32)) {
            huffDone = true;
            ret = blockStatus::FINISH;
        }
        if (bits_cntr < 32 && (done == false)) {
            uint16_t inValue = inStream.read();
            done = inEos.read();
            bitbuffer = (bitbuffer >> (len1 + len2)) | (bitBufferType)(inValue) << bits_cntr;
            bits_cntr += (uint8_t)16;
        } else {
            bitbuffer >>= (len1 + len2);
        }
        if (len_extra) {
            ap_uint<32> val = array_codes_extra[lidx1];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else if (dist_extra) {
            ap_uint<32> val = array_codes_dist_extra[lidx1];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else if (is_length) {
            ap_uint<32> val = array_codes[lidx];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else {
            ap_uint<32> val = array_codes_dist[lidx];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        }
    }
    _bitbuffer = bitbuffer;
    return ret;
}

// Content of called function
T reg(T d) {
#pragma HLS PIPELINE II = 1
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INLINE off
    return d;
}