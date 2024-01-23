void StreamingMaxPool(hls::stream<ap_uint<NumChannels> > & in,
        hls::stream<ap_uint<NumChannels> > & out) {
  static_assert(ImgDim % PoolDim == 0, "");
  // need buffer space for a single maxpooled row of the image
  ap_uint<NumChannels> buf[ImgDim / PoolDim];
  for(unsigned int i = 0; i < ImgDim / PoolDim; i++) {
#pragma HLS UNROLL
    buf[i] = 0;
  }

  for (unsigned int yp = 0; yp < ImgDim / PoolDim; yp++) {
    for (unsigned int ky = 0; ky < PoolDim; ky++) {
      for (unsigned int xp = 0; xp < ImgDim / PoolDim; xp++) {
        ap_uint<NumChannels> acc = 0;
        for (unsigned int kx = 0; kx < PoolDim; kx++) {
#pragma HLS pipeline style=flp II=1
          acc = acc | in.read();
        }
        // pool with old value in row buffer
        buf[xp] |= acc;
      }
    }
    for (unsigned int outpix = 0; outpix < ImgDim / PoolDim; outpix++) {
#pragma HLS pipeline style=flp II=1
      out.write(buf[outpix]);
      // get buffer ready for next use
      buf[outpix] = 0;
    }
  }
}