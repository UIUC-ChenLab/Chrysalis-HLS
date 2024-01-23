void embedding(hls::stream<data_T> &data, hls::stream<res_T> &res,
               typename CONFIG_T::embeddings_t embeddings[CONFIG_T::vocab_size * CONFIG_T::n_out]) {
    data_T in_data = data.read();

InputSequence:
    for (int j = 0; j < data_T::size; j++) {
        #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

        res_T res_pack;
        PRAGMA_DATA_PACK(res_pack)

    DenseEmbedding:
        for (int i = 0; i < CONFIG_T::n_out; i++) {
            #pragma HLS UNROLL
            res_pack[i] = embeddings[in_data[j] * CONFIG_T::n_out + i];
        }
        res.write(res_pack);
    }
}