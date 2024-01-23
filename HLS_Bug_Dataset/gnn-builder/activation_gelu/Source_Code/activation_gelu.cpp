T activation_gelu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    const T sqrt_2_recip = T(1.0) / m_sqrt(T(2.0));
    const T out = x * T(0.5) * (T(1.0) + m_erf(x * sqrt_2_recip));
    return out;
}