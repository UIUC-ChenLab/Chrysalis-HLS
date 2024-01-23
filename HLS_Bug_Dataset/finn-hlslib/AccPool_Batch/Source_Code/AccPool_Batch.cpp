void AccPool_Batch(hls::stream<ap_uint<PECount * ActType::width> > & in,
        hls::stream<ap_uint<PECount * AccType::width> > & out, const unsigned int numReps) {
    ap_uint<PECount * ActType::width> thin;
  ap_uint<PECount * AccType::width> accumulators[NumChannels/PECount];
#pragma HLS bind_storage variable=accumulators type=RAM_2P impl=LUTRAM

    //call to thresholding library function
    for(unsigned int reps=0; reps<numReps; reps++){
        for(unsigned int pixel=0; pixel<ImgDim*ImgDim; pixel++){
      for(unsigned int fold=0; fold<NumChannels/PECount; fold++){
#pragma HLS pipeline style=flp II=1
        thin = in.read();
        ap_uint<PECount * AccType::width> accbank = accumulators[fold];
        for(unsigned int pe=0; pe<PECount; pe++){
#pragma HLS UNROLL
          // Threshold and assign to right bits of output buffers
          ActType const  val = thin((pe+1) * ActType::width - 1,pe * ActType::width);
          AccType const  acc = accbank((pe+1) * AccType::width - 1,pe * AccType::width);
          AccType const  result = val + (pixel == 0? AccType(0) : acc);
          accbank((pe+1) * AccType::width - 1,pe * AccType::width) = result;
        }
        accumulators[fold] = accbank;     
      }
        }
    for (unsigned int fold = 0; fold < NumChannels / PECount; fold++)
    {
      out.write(accumulators[fold]);
    }
    }
}