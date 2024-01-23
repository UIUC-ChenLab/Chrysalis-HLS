T activation_sin(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return m_sin(x);
}