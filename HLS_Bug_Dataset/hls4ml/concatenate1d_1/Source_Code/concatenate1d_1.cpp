void concatenate1d(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    res_T out_data;
    PRAGMA_DATA_PACK(out_data)
ConcatLoop1:
    for (int i = 0; i < CONFIG_T::n_elem1_0 / input1_T::size; i++) {
        #pragma HLS PIPELINE
        input1_T in_data1 = data1.read();
    ConcatPack1:
        for (int j = 0; j < input1_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j + (i * input1_T::size)] = in_data1[j];
        }
    }
ConcatLoop2:
    for (int i = 0; i < CONFIG_T::n_elem2_0 / input2_T::size; i++) {
        #pragma HLS PIPELINE
        input2_T in_data2 = data2.read();
    ConcatPack2:
        for (int j = 0; j < input2_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j + (i * input2_T::size) + (CONFIG_T::n_elem1_0)] = in_data2[j];
        }
    }
    res.write(out_data);
}