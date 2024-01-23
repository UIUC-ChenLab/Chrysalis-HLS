void ConvolutionInputGenerator_NonSquare_Dilated(
		hls::stream<ap_uint<SIMD*Input_precision> > & in,
		hls::stream<ap_uint<SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
  static_assert(IFMChannels % SIMD == 0, "");
  static_assert(Dilation_y == 1, ""); // Dilation on the Y axes not yet supported, available only for API definition

  const unsigned int multiplying_factor = IFMChannels/SIMD;
  const unsigned int number_blocks = (ConvKernelDim_y*Dilation_y)/Stride_y + 1 ;
  ap_uint<SIMD*Input_precision> inputBuf[number_blocks][Stride_x * IFMDim_x * multiplying_factor];

#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
  memory_resource(inputBuf, r);
  const unsigned int cycles_write_block = (OFMDim_x * ConvKernelDim_x * ConvKernelDim_y * multiplying_factor);
  const unsigned int cycles_read_block = Stride_x * IFMDim_x * multiplying_factor;
  const unsigned int max_cycles = std::max(cycles_write_block,cycles_read_block);
  const unsigned int baseIter = IFMDim_x * ConvKernelDim_y * Dilation_y  * multiplying_factor// Initial buffer
			                  + OFMDim_y * std::max(cycles_write_block,cycles_read_block);
  unsigned int counter_internal_block = 0;
  unsigned int current_block_write = 0;
  unsigned int current_line = 0;
  unsigned int read_block = 0;
  unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;

  for (unsigned int count_image = 0; count_image < numReps; count_image++) {
    for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS pipeline style=flp II=1
      if (inp < IFMDim_x * ConvKernelDim_y * Dilation_y *multiplying_factor) {// Initial buffer of ConvKernelDim lines
        ap_uint<SIMD*Input_precision> inElem;
        inElem = in.read();
        inputBuf[current_block_write][current_line] = inElem;
        current_line++;
        inp++;
        if (current_line == Stride_x * IFMDim_x * multiplying_factor ) {
          current_line = 0;
          current_block_write++;
          if (current_block_write == number_blocks) {
            current_block_write=0;
          }
          read_block++;
          counter_internal_block = 0;
        }
      } else {
        if (counter_internal_block < cycles_write_block-1) { // We are writing output, IFMChan per cycle
          unsigned int current_block_read = (current_block_write + 1 + (k_y*Dilation_y) / Stride_y);
          if (current_block_read >= number_blocks) {
            current_block_read-= number_blocks;
		  }
          unsigned int current_line_in_block = ((ofm_x*Stride_x + k_x*Dilation_x)*multiplying_factor + count_simd);
          ap_uint<SIMD*Input_precision> outElem;
          outElem = inputBuf[current_block_read][(current_line_in_block)];
          out.write(outElem);
          count_simd++;
          if (count_simd == multiplying_factor) {
            count_simd=0;
            k_x++;
            if (k_x == ConvKernelDim_x) {
              k_x = 0;
              k_y++;
              if (k_y == ConvKernelDim_y) {
                k_y = 0;
                ofm_x ++;
                if (ofm_x == OFMDim_x) {
                  ofm_x = 0;
                  ofm_y++;
                  if (ofm_y == OFMDim_y) {
                    ofm_y = 0;
                    inp = 0;
                  }
                }
              }
            }
          }
        }
        if ((counter_internal_block < cycles_read_block-1) && (read_block<IFMDim_y/Stride_y)) { // In parallel we write in the buffer, in the current block write if we still need to
          ap_uint<SIMD*Input_precision> inElem;
          inElem = in.read();
          inputBuf[current_block_write][current_line] = inElem;
#pragma AP dependence variable=inputBuf intra false
#pragma AP dependence variable=inputBuf inter false
          current_line++;
          if (current_line == Stride_x * IFMDim_x * multiplying_factor) {// We read the whole block, we change the next block in which we want to we
            // We filled up a block, let's not read until
            current_line = 0;
            read_block++;
            current_block_write++;
            if (current_block_write == number_blocks) {
              current_block_write=0;
			}
#pragma AP dependence variable=current_block_write intra false
          }
        }
        counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
        if (counter_internal_block == (max_cycles-1)) {
          counter_internal_block = 0;
        }
      }
    } // End base_iter
	read_block = 0;
  } // End count_image
}

// Content of called function
void memory_resource(T inputBuf, ap_resource_lutram const&){
#pragma HLS BIND_STORAGE variable=inputBuf type=RAM_S2P impl=LUTRAM
}