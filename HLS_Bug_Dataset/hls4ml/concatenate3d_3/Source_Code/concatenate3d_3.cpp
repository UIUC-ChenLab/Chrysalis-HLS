void concatenate3d(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    if (CONFIG_T::axis == 3 || CONFIG_T::axis == -1) {
        concatenate3d_2<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    } else if (CONFIG_T::axis == 2 || CONFIG_T::axis == -2) {
        concatenate3d_1<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    } else {
        concatenate3d_0<input1_T, input2_T, res_T, CONFIG_T>(data1, data2, res);
    }
}

// Content of called function
void concatenate3d_2(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
ConcatLoopHeight:
    for (int i = 0; i < CONFIG_T::n_elem1_0; i++) {
    ConcatLoopWidth:
        for (int j = 0; j < CONFIG_T::n_elem1_1; j++) {
            #pragma HLS PIPELINE II=1

            input1_T in_data1 = data1.read();
            input2_T in_data2 = data2.read();
            res_T out_data;
            PRAGMA_DATA_PACK(out_data)

        ConcatPackInput1:
            for (int k = 0; k < input1_T::size; k++) {
                #pragma HLS UNROLL
                out_data[k] = in_data1[k];
            }

        ConcatPackInput2:
            for (int k = 0; k < input2_T::size; k++) {
                #pragma HLS UNROLL
                out_data[input1_T::size + k] = in_data2[k];
            }

            res.write(out_data);
        }
    }
}

// Content of called function
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

// Content of called function
void concatenate3d_0(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
ConcatLoopHeight1:
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
    }
ConcatLoopHeight2:
    for (int i = 0; i < CONFIG_T::n_elem2_0; i++) {
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