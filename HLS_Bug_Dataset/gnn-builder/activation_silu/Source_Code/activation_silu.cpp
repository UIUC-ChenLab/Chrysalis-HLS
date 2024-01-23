T activation_silu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return x * (T(1.0) / (T(1.0) + m_exp(-x)));
}