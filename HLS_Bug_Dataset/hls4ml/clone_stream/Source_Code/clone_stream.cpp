void clone_stream(hls::stream<data_T> &data, hls::stream<res_T> &res1, hls::stream<res_T> &res2) {
CloneLoop:
    for (int i = 0; i < N / data_T::size; i++) {
        #pragma HLS PIPELINE

        data_T in_data = data.read();
        res_T out_data1;
        res_T out_data2;
        PRAGMA_DATA_PACK(out_data1)
        PRAGMA_DATA_PACK(out_data2)

    ClonePack:
        for (int j = 0; j < data_T::size; j++) {
            #pragma HLS UNROLL
            out_data1[j] = in_data[j];
            out_data2[j] = in_data[j];
        }

        res1.write(out_data1);
        res2.write(out_data2);
    }
}