void mean_incremental_update(mean_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.sum += x;
    data.count++;
    data.mean = data.sum / T(data.count);
}