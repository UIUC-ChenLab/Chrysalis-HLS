T activation_leakyrelu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    const T negative_slope = T(0.1);

    if (x >= 0)
    {
        return x;
    }
    else
    {
        return x * negative_slope;
    }
}