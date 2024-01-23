void ConvolutionInputGenerator_kernel_stride_MMV(  
		hls::stream<ap_uint<SIMD*Input_precision> > & in,
		hls::stream<MultiChanData<MMV, SIMD*Input_precision> > & out,
		const unsigned int numReps,
		R const &r) {
	static_assert(IFMChannels % SIMD == 0, "");
	static_assert(ConvKernelDim % Stride != 0, "");
	static_assert(OFMDim % MMV == 0, "");
	static_assert(MMV <= OFMDim, "");

	const unsigned int multiplying_factor = IFMChannels/SIMD;
	const unsigned int number_blocks = ConvKernelDim + Stride ;
	ap_uint<SIMD*Input_precision> inputBuf[MMV][number_blocks][IFMDim * multiplying_factor];
#pragma HLS DEPENDENCE variable=inputBuf inter false
#pragma HLS DEPENDENCE variable=inputBuf intra false
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=1
#pragma HLS ARRAY_PARTITION variable=inputBuf complete dim=2
    memory_resource(inputBuf, r);
	const unsigned int cycles_write_block = (OFMDim * ConvKernelDim * ConvKernelDim * multiplying_factor)/MMV;
	const unsigned int cycles_read_block = IFMDim * Stride * multiplying_factor;
	const unsigned int max_cycles = std::max(cycles_write_block, cycles_read_block);
	const unsigned int baseIter = (IFMDim * ConvKernelDim * multiplying_factor) + (OFMDim-1) * max_cycles+std::max(cycles_write_block,OFMDim);
	const unsigned int initial_buffer_cycles = (IFMDim * ConvKernelDim * multiplying_factor) ;
	unsigned int counter_internal_block = 0;
	unsigned int current_line = 0;

	unsigned int inp = 0, ofm_y = 0, ofm_x = 0, k_y = 0, k_x = 0, count_simd =0;

for (unsigned int count_image = 0; count_image < numReps; count_image++) {
  unsigned int floor_block_read = 0, ceil_block_read = number_blocks;
  unsigned int current_block_write = 0;
#pragma HLS DEPENDENCE variable=current_block_write intra false
  unsigned int read_block = 0;
		for (unsigned int i = 0; i < baseIter; i++) {
#pragma HLS pipeline style=flp II=1
			if (inp < initial_buffer_cycles) // Initial buffer of PoolDim lines
			{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++)
					{
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				inp++;
				if (current_line == IFMDim * multiplying_factor)
				{
					current_line = 0;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write = 0;
					read_block++;
					counter_internal_block = 0;
				}
			}
			else
			{
				if (counter_internal_block < cycles_write_block-1 || read_block==IFMDim) // We are writing output, MMV IFMChan per cycle
				{
					//following code implements: current_block_read = (ofm_y*Stride + k_y)%number_blocks;
            unsigned int current_block_read = (ofm_y*Stride + k_y);
            //reminder computation
            if (current_block_read >= ceil_block_read)
            {
              floor_block_read += number_blocks;
              ceil_block_read += number_blocks;
            }else if(current_block_read < floor_block_read){
              ceil_block_read -= number_blocks;
              floor_block_read -= number_blocks;
            }
            current_block_read -= floor_block_read;
			unsigned int current_line_in_block = (ofm_x * Stride + k_x)*multiplying_factor + count_simd;
			MultiChanData<MMV, SIMD*Input_precision> outElem;
			for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
				// each buffer's read addr is offset by its buffer index
				ap_uint<SIMD*Input_precision> temp_value = inputBuf[v][current_block_read][(current_line_in_block + v*Stride*multiplying_factor)];
				outElem.data[v] = temp_value;
			}
			out.write(outElem);
			count_simd++;
			if (count_simd == multiplying_factor) {
				count_simd=0;	
				k_x++;
				if (k_x == ConvKernelDim) {
					k_x = 0;
					k_y++;
					if (k_y == ConvKernelDim) {
						k_y = 0;
						ofm_x += MMV;
						if (ofm_x == OFMDim) {
							ofm_x = 0;
							ofm_y++;
							if (ofm_y == OFMDim) {
								ofm_y = 0;
								inp = 0;
								}
							}
						}
					}
				}
			}
			if ((counter_internal_block < cycles_read_block - 1) && (read_block<IFMDim)) // In parallel we write in the buffer, in the current block write if we still need to
			{
				ap_uint<SIMD*Input_precision> inElem;
				inElem = in.read();
				for(unsigned int v = 0; v < MMV; v++) {
#pragma HLS UNROLL
					inputBuf[v][current_block_write][current_line] = inElem;
					}
				current_line++;
				if (current_line == IFMDim * multiplying_factor) // We read the whole block, we change the next block in which we want to we
				{ // We filled up a block, let's not read until
					current_line = 0;
					read_block++;
					current_block_write++;
					if (current_block_write == number_blocks)
						current_block_write = 0;
#pragma HLS DEPENDENCE variable=current_block_write intra false
				}
			}
			counter_internal_block++; // = (counter_internal_block +1) % max_cycles;
			if (counter_internal_block == (max_cycles-1))
			{
			   counter_internal_block = 0;
			}
		}
	} // End base_iter
  }
}

// Content of called function
void memory_resource(T inputBuf, ap_resource_lutram const&){
#pragma HLS BIND_STORAGE variable=inputBuf type=RAM_S2P impl=LUTRAM
}