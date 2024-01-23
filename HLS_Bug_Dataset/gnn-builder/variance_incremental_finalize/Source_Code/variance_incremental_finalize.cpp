void variance_incremental_finalize(variance_incremental_data<T> &data)
{
    // #pragma HLS INLINE off
    data.var = data.m2 / T(data.count);
    data.std = m_sqrt(data.var + T(1e-5));
    data.finalized = true;
}