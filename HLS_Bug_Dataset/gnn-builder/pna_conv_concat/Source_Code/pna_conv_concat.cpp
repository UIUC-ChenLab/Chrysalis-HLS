void pna_conv_concat(
    T current_node_embedding_in[EMB_SIZE_IN],
    T agg_max_identity_emb[EMB_SIZE_IN],
    T agg_min_identity_emb[EMB_SIZE_IN],
    T agg_mean_identity_emb[EMB_SIZE_IN],
    T agg_std_identity_emb[EMB_SIZE_IN],
    T agg_max_amplification[EMB_SIZE_IN],
    T agg_min_amplification[EMB_SIZE_IN],
    T agg_mean_amplification[EMB_SIZE_IN],
    T agg_std_amplification[EMB_SIZE_IN],
    T agg_max_attenuation[EMB_SIZE_IN],
    T agg_min_attenuation[EMB_SIZE_IN],
    T agg_mean_attenuation[EMB_SIZE_IN],
    T agg_std_attenuation[EMB_SIZE_IN],
    T pre_apply_emb[CONCAT_SIZE])
{
#pragma HLS INLINE off
    for (int i = 0; i < EMB_SIZE_IN; i++)
    {
        pre_apply_emb[i + EMB_SIZE_IN * 0] = current_node_embedding_in[i];

        pre_apply_emb[i + EMB_SIZE_IN * 1] = agg_max_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 2] = agg_min_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 3] = agg_mean_identity_emb[i];
        pre_apply_emb[i + EMB_SIZE_IN * 4] = agg_std_identity_emb[i];

        pre_apply_emb[i + EMB_SIZE_IN * 5] = agg_max_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 6] = agg_min_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 7] = agg_mean_amplification[i];
        pre_apply_emb[i + EMB_SIZE_IN * 8] = agg_std_amplification[i];

        pre_apply_emb[i + EMB_SIZE_IN * 9] = agg_max_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 10] = agg_min_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 11] = agg_mean_attenuation[i];
        pre_apply_emb[i + EMB_SIZE_IN * 12] = agg_std_attenuation[i];
    }
}