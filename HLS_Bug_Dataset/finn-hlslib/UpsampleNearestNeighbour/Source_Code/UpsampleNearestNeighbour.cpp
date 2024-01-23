void UpsampleNearestNeighbour(
        hls::stream<ap_uint<NumChannels * In_t::width>> & in,
        hls::stream<ap_uint<NumChannels * In_t::width>> & out
) {
  static_assert(OFMDim > IFMDim, "");

  constexpr unsigned int scale_factor = OFMDim/IFMDim;
  constexpr unsigned int Padding = OFMDim % IFMDim;
  // Padding might be asymmetrical
  constexpr unsigned int PaddingDown = Padding/2;
  constexpr unsigned int PaddingUp = Padding - PaddingDown;
  // Padding might be asymmetrical
  constexpr unsigned int PaddingRight = Padding/2;
  constexpr unsigned int PaddingLeft = Padding - PaddingRight;

  ap_uint<NumChannels * In_t::width> outData, inData;
  ap_uint<NumChannels * In_t::width> RowBuf[IFMDim];
  int count_row = -PaddingUp; // Counter used to understand whether reading (and buffering) a row or not - Made in order to avoid modulo operations
  for (unsigned int y = 0; y < OFMDim; y++) {
	  for (unsigned int x = 0; x < OFMDim; x++) {
#pragma HLS pipeline style=flp II=1
		bool read_row = (y ==0) || count_row==scale_factor;
		if ((x < IFMDim) && read_row)
		{
			inData = in.read();
			RowBuf[x] = inData;
		}
		// Padding Cols
		if(x < PaddingLeft){
			outData = RowBuf[0];
		}
		else if (x >= (OFMDim - PaddingRight)){
			outData = RowBuf[IFMDim-1];

		}
		// Padding Rows
		else if(y < PaddingUp || y >= (OFMDim - PaddingDown)){
			outData = RowBuf[(x-PaddingLeft)/scale_factor];
		}
		// No Padding
		else{

			outData = RowBuf[(x-PaddingLeft)/scale_factor];
		}
		//std::cout << outData << " " ;
		out.write(outData);
	  }// end for y
	  //std::cout << std::endl;
	  count_row++;
	  if (count_row > scale_factor)
		  count_row =1;
  } // end for x

}