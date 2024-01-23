void StreamingMaxPool_Precision_1d(hls::stream<ap_uint<PE*ActType::width> > & in,
        hls::stream<ap_uint<PE*ActType::width> > & out) {
  static_assert(NumChannels % PE == 0, "");
  constexpr unsigned NF = NumChannels / PE;
  constexpr unsigned REMAINDER_PIXELS = ImgDim > PoolDim * OutputSize ? ImgDim - OutputSize * PoolDim : 0;
  
  // need buffer space for a single maxpooled pixel of the image
  ActType buf[NF][PE];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=2

  for(unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
    for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
        buf[ch][p] = min_value;
    }
  }

  ap_uint<PE*ActType::width> inputData,outputData;
  unsigned input_count = 0;
  for (unsigned int xp = 0; xp < OutputSize; xp++) {
    // Change to comparator
    for (unsigned int kx = 0; kx < PoolDim; kx++) {
      if (input_count++ < ImgDim){
        for (unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
          inputData = in.read();
          for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
            unsigned const lowBit = p * ActType::width;
            unsigned const highBit = (p+1) * ActType::width -1;
            ActType const channeldata = inputData(highBit, lowBit);
            ActType const oldMax = buf[ch][p];
            if(channeldata > oldMax){
              buf[ch][p] = channeldata;
            }
          }
        }
      }
    }
    for(unsigned int ch = 0; ch < NF; ch++){
#pragma HLS pipeline style=flp II=1
      for(unsigned int p = 0; p < PE; p++){
#pragma HLS UNROLL
        unsigned const lowBit = p * ActType::width;
        unsigned const highBit = (p+1) * ActType::width -1;
        outputData(highBit, lowBit) = buf[ch][p];
        // get buffer ready for next use
        buf[ch][p] = min_value;
      }
      out.write(outputData);
    }
  }

  for (unsigned int r = 0; r < REMAINDER_PIXELS*NF; r++){
#pragma HLS pipeline style=flp II=1
      inputData = in.read();
  }

}