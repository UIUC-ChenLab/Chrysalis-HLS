void embedding(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_in * CONFIG_T::n_out],
               typename CONFIG_T::embeddings_t embeddings[CONFIG_T::vocab_size * CONFIG_T::n_out]) {

    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor
    // This can save a few cycles, but it will create a large multiplexer due to
    // non-constant access pattern, so let's leave it out
    //#pragma HLS ARRAY_PARTITION variable=embeddings complete

InputSequence:
    for (int j = 0; j < CONFIG_T::n_in; j++) {
    #pragma HLS UNROLL
    DenseEmbedding:
        for (int i = 0; i < CONFIG_T::n_out; i++) {
            #pragma HLS UNROLL
            res[j * CONFIG_T::n_out + i] = embeddings[data[j] * CONFIG_T::n_out + i];
        }
    }
}