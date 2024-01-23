void max_incremental_finalize(max_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.finalized = true;
}