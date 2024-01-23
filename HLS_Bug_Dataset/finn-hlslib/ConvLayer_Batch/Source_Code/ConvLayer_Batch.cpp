void ConvLayer_Batch(hls::stream<ap_uint<InStreamW>>  &in,
			    hls::stream<ap_uint<OutStreamW>> &out,
			    TW const        &weights,
			    TA const        &activation,
			    unsigned const   reps,
				R const &r) {
#pragma HLS INLINE
  unsigned const MatrixW = ConvKernelDim * ConvKernelDim * IFMChannels;
  unsigned const MatrixH = OFMChannels;
  unsigned const InpPerImage = IFMDim * IFMDim * IFMChannels * TSrcI::width / InStreamW;
  hls::stream<ap_uint<SIMD*TSrcI::width> > wa_in("StreamingConvLayer_Batch.wa_in");
  hls::stream<ap_uint<SIMD*TSrcI::width> > convInp("StreamingConvLayer_Batch.convInp");
  hls::stream<ap_uint<PE*TDstI::width> > mvOut("StreamingConvLayer_Batch.mvOut");
  StreamingDataWidthConverter_Batch<InStreamW, SIMD*TSrcI::width, InpPerImage>(in, wa_in, reps);
  ConvolutionInputGenerator<ConvKernelDim, IFMChannels, TSrcI::width, IFMDim,
			OFMDim, SIMD,1>(wa_in, convInp, reps, ap_resource_dflt());
  Matrix_Vector_Activate_Batch<MatrixW, MatrixH, SIMD, PE, 1, TSrcI, TDstI, TWeightI>
    (static_cast<hls::stream<ap_uint<SIMD*TSrcI::width>>&>(convInp),
     static_cast<hls::stream<ap_uint<PE*TDstI::width>>&>  (mvOut),
     weights, activation, reps* OFMDim * OFMDim, r);
  StreamingDataWidthConverter_Batch<PE*TDstI::width, OutStreamW, OFMDim * OFMDim * (OFMChannels / PE)>(mvOut, out, reps);

}