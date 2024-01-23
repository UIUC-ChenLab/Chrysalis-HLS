T activation_cos(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return m_cos(x);
}