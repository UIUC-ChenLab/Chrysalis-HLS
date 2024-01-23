void mean_incremental_finalize(mean_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}