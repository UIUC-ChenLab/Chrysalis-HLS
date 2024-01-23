T activation_identity(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return x;
}