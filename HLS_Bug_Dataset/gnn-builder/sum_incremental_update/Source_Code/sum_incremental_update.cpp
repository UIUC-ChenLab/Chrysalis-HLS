void sum_incremental_update(sum_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
}