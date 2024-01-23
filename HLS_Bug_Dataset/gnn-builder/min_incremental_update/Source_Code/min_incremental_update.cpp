void min_incremental_update(min_incremental_data<T> &data, T x)
{
    if (data.one_sample_processed == false)
    {
        data.min = x;
        data.one_sample_processed = true;
    }
    else
    {
        if (x < data.min)
        {
            data.min = x;
        }
    }
}