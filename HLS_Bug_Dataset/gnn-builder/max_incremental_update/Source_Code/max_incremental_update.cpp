void max_incremental_update(max_incremental_data<T> &data, T x)
{
    // #pragma HLS INLINE off
    if (data.one_sample_processed == false)
    {
        data.max = x;
        data.one_sample_processed = true;
    }
    else
    {
        if (x > data.max)
        {
            data.max = x;
        }
    }
}