void concatenate3d_1(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
ConcatLoopHeight:
    for (int i = 0; i < CONFIG_T::n_elem1_0; i++) {
    ConcatLoopWidth1:
        for (int j = 0; j < CONFIG_T::n_elem1_1; j++) {
            #pragma HLS PIPELINE II=1

            input1_T in_data1 = data1.read();
            res_T out_data;
            PRAGMA_DATA_PACK(out_data)

        ConcatPackInput1:
            for (int k = 0; k < input1_T::size; k++) {
                #pragma HLS UNROLL
                out_data[k] = in_data1[k];
            }

            res.write(out_data);
        }
    ConcatLoopWidth2:
        for (int j = 0; j < CONFIG_T::n_elem2_1; j++) {
            #pragma HLS PIPELINE II=1

            input2_T in_data2 = data2.read();
            res_T out_data;
            PRAGMA_DATA_PACK(out_data)

        ConcatPackInput2:
            for (int k = 0; k < input2_T::size; k++) {
                #pragma HLS UNROLL
                out_data[k] = in_data2[k];
            }

            res.write(out_data);
        }
    }
}