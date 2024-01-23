void AddStreamsLayer_Batch(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                           hls::stream<ap_uint<NumChannels * Out_t::width>> &out, const unsigned int numReps) {
#pragma HLS INLINE
  static_assert(NumChannels % PECount == 0, "");
  hls::stream<ap_uint<PECount * In1_t::width>> in_folded1;
  hls::stream<ap_uint<PECount * In2_t::width>> in_folded2;
  hls::stream<ap_uint<PECount * Out_t::width>> out_folded;
  StreamingDataWidthConverter_Batch<NumChannels * In1_t::width, PECount * In1_t::width, NumTotal>(in1, in_folded1, numReps);
  StreamingDataWidthConverter_Batch<NumChannels * In2_t::width, PECount * In2_t::width, NumTotal>(in2, in_folded2, numReps);
  AddStreams_Batch<PECount, In1_t, In2_t, Out_t, NumTotal *(NumChannels / PECount),offset>(in_folded1, in_folded2, out_folded, numReps);
  StreamingDataWidthConverter_Batch<PECount * Out_t::width, NumChannels * Out_t::width, NumTotal *(NumChannels / PECount)>(out_folded, out, numReps);
}

// Content of called function
void StreamingDataWidthConverter_Batch(hls::stream<ap_uint<InWidth> > & in,
		hls::stream<ap_uint<OutWidth> > & out, const unsigned int numReps) {
  static_assert((InWidth % OutWidth == 0) || (OutWidth % InWidth == 0), "");

  if (InWidth > OutWidth) {
    // emit multiple output words per input word read
    const unsigned int outPerIn = InWidth / OutWidth;
    const unsigned int totalIters = NumInWords * outPerIn * numReps;
    unsigned int o = 0;
    ap_uint<InWidth> ei = 0;
    for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
      // read new input word if current out count is zero
      if (o == 0) {
        ei = in.read();
	  }
      // pick output word from the rightmost position
      ap_uint<OutWidth> eo = ei(OutWidth - 1, 0);
      out.write(eo);
      // shift input to get new output word for next iteration
      ei = ei >> OutWidth;
      // increment written output count
      o++;
      // wraparound indices to recreate the nested loop structure
      if (o == outPerIn) {
        o = 0;
      }
    }
  } else if (InWidth == OutWidth) {
    // straight-through copy
    for (unsigned int i = 0; i < NumInWords * numReps; i++) {
#pragma HLS pipeline style=flp II=1
      ap_uint<InWidth> e = in.read();
      out.write(e);
    }
  } else { // InWidth < OutWidth
    // read multiple input words per output word emitted
    const unsigned int inPerOut = OutWidth / InWidth;
    const unsigned int totalIters = NumInWords * numReps;
    unsigned int i = 0;
    ap_uint<OutWidth> eo = 0;
    for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
      // read input and shift into output buffer
      ap_uint<InWidth> ei = in.read();
      eo = eo >> InWidth;
      eo(OutWidth - 1, OutWidth - InWidth) = ei;
      // increment read input count
      i++;
      // wraparound logic to recreate nested loop functionality
      if (i == inPerOut) {
        i = 0;
        out.write(eo);
      }
    }
  }
}

// Content of called function
void AddStreams_Batch(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                hls::stream<ap_uint<NumChannels * Out_t::width>> &out, const unsigned int numReps) {
  for (unsigned int image = 0; image < numReps; image++) {
    AddStreams<NumChannels, In1_t, In2_t, Out_t, NumTotal, offset>(in1, in2, out);
  }
}

// Content of called function
void AddStreams(hls::stream<ap_uint<NumChannels * In1_t::width>> &in1, hls::stream<ap_uint<NumChannels * In2_t::width>> &in2,
                hls::stream<ap_uint<NumChannels * Out_t::width>> &out) {

  for (unsigned int i = 0; i < NumTotal; i++) {
#pragma HLS pipeline style=flp II=1
    ap_uint<NumChannels * In1_t::width> e1 = in1.read();
    ap_uint<NumChannels * In2_t::width> e2 = in2.read();
    ap_uint<NumChannels * Out_t::width> e;
    for (unsigned int j = 0; j < NumChannels; j++) {
#pragma HLS UNROLL
      In1_t op1 = e1((j + 1) * In1_t::width - 1, j * In1_t::width);
      In2_t op2 = e2((j + 1) * In2_t::width - 1, j * In2_t::width);
      Out_t sum = op1 + op2 + offset;
      e((j + 1) * Out_t::width - 1, j * Out_t::width) = sum;
    }
    out.write(e);
  }
}