void ConvLayer_Batch_TMR(hls::stream<ap_uint<InStreamW>>  &in,
                         hls::stream<ap_uint<OutStreamW>> &out,
                         TW const        &weights,
                         TA const        &activation,
                         unsigned const   reps,
                         R const &r,
                         ap_uint<2> &errortype,
                         ap_uint<OFMChannels> channel_mask,
                         ap_uint<MAX_CH_WIDTH> red_cha_index[NUM_RED]) {
#pragma HLS INLINE
  unsigned const MatrixW = ConvKernelDim * ConvKernelDim * IFMChannels;
  unsigned const MatrixH = OFMChannels;
  unsigned const InpPerImage = IFMDim*IFMDim;

  hls::stream<ap_uint<SIMD*TSrcI::width> > wa_in("StreamingConvLayer_Batch.wa_in");
  hls::stream<ap_uint<SIMD*TSrcI::width> > convInp("StreamingConvLayer_Batch.convInp");
  hls::stream<ap_uint<PE*TDstI::width> > mvOut("StreamingConvLayer_Batch.mvOut");
  hls::stream<ap_uint<OFMChannels*TDstI::width> > tmr_in("StreamingConvLayer_Batch.tmr_in");

  StreamingDataWidthConverter_Batch<InStreamW, SIMD*TSrcI::width, InpPerImage>(in, wa_in, reps);

  //Sliding window unit
  ConvolutionInputGenerator<ConvKernelDim, IFMChannels, TSrcI::width, IFMDim,
            OFMDim, SIMD,1>(wa_in, convInp, reps, ap_resource_dflt());

  //MVTU
  Matrix_Vector_Activate_Batch<MatrixW, MatrixH, SIMD, PE, 1, TSrcI, TDstI, TWeightI>
    (static_cast<hls::stream<ap_uint<SIMD*TSrcI::width>>&>(convInp),
     static_cast<hls::stream<ap_uint<PE*TDstI::width>>&>  (mvOut),
     weights, activation, reps* OFMDim * OFMDim, r);

  StreamingDataWidthConverter_Batch<PE*TDstI::width, OFMChannels*TDstI::width, OFMDim * OFMDim * (OFMChannels / PE)>(mvOut, tmr_in, reps);

  //Error check
  TMRCheck_Batch<TDstI::width, OFMChannels, NUM_RED, REDF, OFMDim, MAX_CH_WIDTH>(tmr_in, out, errortype, channel_mask, red_cha_index, reps);
}