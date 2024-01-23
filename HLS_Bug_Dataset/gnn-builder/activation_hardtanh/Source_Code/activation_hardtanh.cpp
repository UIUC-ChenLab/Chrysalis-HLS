T activation_hardtanh(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    const T min_val = T(-1.0);
    const T max_val = T(1.0);
    if (x < min_val)
    {
        return min_val;
    }
    else if (x > max_val)
    {
        return max_val;
    }
    else
    {
        return x;
    }
}