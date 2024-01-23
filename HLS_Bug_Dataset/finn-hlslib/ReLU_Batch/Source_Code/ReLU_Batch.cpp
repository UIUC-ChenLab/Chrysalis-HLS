void ReLU_Batch(hls::stream<ap_uint<PECount * ActType::width> > & in,
        hls::stream<ap_uint<PECount * ActType::width> > & out, const unsigned int numReps) {

    ap_uint<PECount * ActType::width> thin;
    ap_uint<PECount * ActType::width> thout;
    
    //call to thresholding library function
    for(unsigned int reps=0; reps<numReps; reps++){
        for(unsigned int pixel=0; pixel<ImgDim*ImgDim; pixel++){
      for(unsigned int fold=0; fold<NumChannels/PECount; fold++){
#pragma HLS pipeline style=flp II=1
        thin = in.read();
        for(unsigned int pe=0; pe<PECount; pe++){
#pragma HLS UNROLL
          // Threshold and assign to right bits of output buffers
          unsigned int lowBit = pe * ActType::width;
          unsigned int highBit = (pe+1) * ActType::width - 1;
          ActType val = thin(highBit,lowBit);
          ActType result;
          if(val < offset)
                  result = 0;
          else
                  result = val - offset;
          thout(highBit, lowBit) = result;
        }    
        out.write(thout);
      }
        }
    }
}