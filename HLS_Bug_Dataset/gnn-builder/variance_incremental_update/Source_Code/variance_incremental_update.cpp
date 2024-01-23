void variance_incremental_update(variance_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    data.count++;
    T delta = x - data.mean;
    data.mean += delta / T(data.count);
    data.m2 += delta * (x - data.mean);
}