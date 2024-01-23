void sum_incremental_finalize(sum_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}