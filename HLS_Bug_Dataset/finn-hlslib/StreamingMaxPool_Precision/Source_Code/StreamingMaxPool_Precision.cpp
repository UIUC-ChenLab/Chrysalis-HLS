void StreamingMaxPool_Precision(hls::stream<ap_uint<StreamW> > & in,
        hls::stream<ap_uint<StreamW> > & out) {
  static_assert(ImgDim % PoolDim == 0, "");
  // need buffer space for a single maxpooled row of the image
  ActType buf[ImgDim / PoolDim][NumChannels];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=2
  for(unsigned int i = 0; i < ImgDim / PoolDim; i++) {
    for(unsigned int ch = 0; ch<NumChannels; ch++){
#pragma HLS UNROLL
      buf[i][ch] = min_value; //std::numeric_limits<ActType>::min();
    }
  }
  ap_uint<StreamW> inputData,outputData;
  for (unsigned int yp = 0; yp < ImgDim / PoolDim; yp++) {
    for (unsigned int ky = 0; ky < PoolDim; ky++) {
      for (unsigned int xp = 0; xp < ImgDim / PoolDim; xp++) {
        // Change to comparator 
        for (unsigned int kx = 0; kx < PoolDim; kx++) {
#pragma HLS pipeline style=flp II=1
          inputData = in.read();
          for(unsigned int ch = 0; ch<NumChannels; ch++){
#pragma HLS UNROLL                      
            unsigned int lowBit = ch * ActType::width;
            unsigned int highBit = (ch+1) * ActType::width -1;
            ActType channeldata = inputData(highBit, lowBit);                   
            ActType oldMax = buf[xp][ch];               
            if(channeldata > oldMax){
              buf[xp][ch] = channeldata;
            }
          }
        }
      }
    }
    for (unsigned int outpix = 0; outpix < ImgDim / PoolDim; outpix++) {
      for(unsigned int ch = 0; ch < NumChannels; ch++){
#pragma HLS UNROLL
        unsigned int lowBit = ch * ActType::width;
        unsigned int highBit = (ch+1) * ActType::width -1;  
        outputData(highBit, lowBit) = buf[outpix][ch];
        // get buffer ready for next use
        buf[outpix][ch] = min_value;
      }
      out.write(outputData);
    }
  }
}