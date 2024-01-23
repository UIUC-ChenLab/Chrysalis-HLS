T activation_relu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    if (x > 0)
    {
        return x;
    }
    else
    {
        return 0;
    }
}