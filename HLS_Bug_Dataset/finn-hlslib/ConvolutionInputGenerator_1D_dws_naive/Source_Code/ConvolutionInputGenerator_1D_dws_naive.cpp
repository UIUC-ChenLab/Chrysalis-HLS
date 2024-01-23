void ConvolutionInputGenerator_1D_dws_naive(
		hls::stream<ap_uint<SIMD*Input_precision> > & in,
		hls::stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  static_assert(IFMChannels % SIMD == 0, "");

  constexpr unsigned multiplying_factor = IFMChannels / SIMD;
  constexpr unsigned cycles_write_block = OFMDim_x * ConvKernelDim_x * multiplying_factor;
  constexpr unsigned cycles_read_block = IFMDim_x * multiplying_factor;
  ap_uint<SIMD*Input_precision> inputBuf[cycles_read_block];
  memory_resource(inputBuf, r);
  constexpr unsigned baseIter = cycles_read_block // Initial buffer
			                  + cycles_write_block;
  unsigned int current_line = 0;
  unsigned int inp = 0, ofm_x = 0, k_x = 0, count_simd =0;

  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS pipeline style=flp II=1
      if (inp < cycles_read_block) {// Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
		inputBuf[current_line] = inElem;
        current_line++;
        inp++;
      }
      else {
            unsigned int current_line_in_block = (ofm_x * Stride_x + k_x * Dilation_x) * multiplying_factor + count_simd;
            ap_uint<SIMD*Input_precision> outElem = inputBuf[current_line_in_block];
            out.write(outElem);
            k_x++;
            if (k_x == ConvKernelDim_x) {
                k_x = 0;
                count_simd++;
                if (count_simd == multiplying_factor) {
                    count_simd=0;
                    ofm_x++;
                    if (ofm_x == OFMDim_x) {
                        ofm_x = 0;
                        inp = 0;
                    }
                }
            }
      } // End else
    } // End base_iter
  } // End image
}

// Content of called function
void memory_resource(T inputBuf, ap_resource_lutram const&){
#pragma HLS BIND_STORAGE variable=inputBuf type=RAM_S2P impl=LUTRAM
}