void GenParamStream(TW const &W_in, hls::stream<ap_uint<SIMD * PE * WP>> &paramStreamOut, int const numReps) {
  for (unsigned rep = 0; rep < numReps; rep++) {
    for (unsigned tile = 0; tile < TILES; tile++) {
#pragma HLS pipeline style=flp II=1

      ap_uint<SIMD * PE * WP> strMem;
      for (unsigned pe = 0; pe < PE; pe++) {
        // Concatenate the weights within the tile into a large SIMD * PE * WP wide word
        // Using Little Endian PE order
        strMem((SIMD * WP)*(pe+1)-1,(SIMD * WP)*pe) = W_in.m_weights[pe][tile]; 
      }

      paramStreamOut.write(strMem);
    }
  }
}