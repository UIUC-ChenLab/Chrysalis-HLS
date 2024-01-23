void ConvLayer_Batch_MMV(hls::stream<ap_uint<InStreamW>>  &in,
			    hls::stream<ap_uint<OutStreamW>> &out,
			    TW const        &weights,
			    TA const        &activation,
			    unsigned const   reps,
				R const &r) {
#pragma HLS INLINE
  unsigned const MatrixW = ConvKernelDim * ConvKernelDim * IFMChannels;
  unsigned const MatrixH = OFMChannels;
  unsigned const InpPerImage = IFMDim*IFMDim*IFMChannels * TSrcI::width/InStreamW;
  const unsigned int mmvReps = (reps * OFMDim * OFMDim) / MMV;

  hls::stream<ap_uint<SIMD * TSrcI::width> > wa_in("StreamingConvLayerMMV_Batch.wa_in");
  hls::stream<MultiChanData<MMV, SIMD *TSrcI::width> > convInp("StreamingConvLayerMMV_Batch.convInp");
  hls::stream<MultiChanData<MMV, PE * TDstI::width> > mmv2dwc("StreamingConvLayerMMV_Batch.mmv2dwc");
  hls::stream<MultiChanData<MMV, OFMChannels * TDstI::width>> dwc2flat("dwc2flat");
  hls::stream<ap_uint<MMV * OFMChannels * TDstI::width> > mvOut("StreamingConvLayerMMV_Batch.mvOut");

  StreamingDataWidthConverter_Batch<InStreamW, SIMD * TSrcI::width, InpPerImage>(in, wa_in, reps);

  ConvolutionInputGenerator_MMV<ConvKernelDim, IFMChannels, TSrcI::width, IFMDim,
			OFMDim, SIMD, STRIDE, MMV>(wa_in, convInp, reps, ap_resource_dflt());
  Matrix_Vector_Activate_Batch<MatrixW, MatrixH, SIMD, PE, MMV, TSrcI, TDstI, TWeightI>
    (static_cast<hls::stream<MultiChanData<MMV,SIMD*TSrcI::width>>&>(convInp),
     static_cast<hls::stream<MultiChanData<MMV,PE*TDstI::width>>&>(mmv2dwc),
     weights, activation, mmvReps, r);
  
  MultiChanDataWidthConverter_Batch<PE * TDstI::width, OFMChannels * TDstI::width, OFMDim * OFMDim * (OFMChannels / PE), MMV>(mmv2dwc, dwc2flat, reps);
  FlattenMultiChanData<MMV, OFMChannels * TDstI::width>(dwc2flat, mvOut, mmvReps);
  StreamingDataWidthConverter_Batch<MMV * OFMChannels * TDstI::width, OutStreamW, OFMDim * OFMDim / MMV>(mvOut, out, reps);
  
}